/*!
 * \file
 * \brief
 * \author Lukasz Zmuda
 */
#include "ObjectRemover.hpp"

#define siftPointSize 133
#define xyzPointSize 3
#define xyzrgbPointSize 4

namespace Processors {
namespace ObjectRemover  {
using namespace cv;
using namespace mongo;
using namespace boost::property_tree;

ObjectRemover::ObjectRemover(const std::string & name) : Base::Component(name),
		mongoDBHost("mongoDBHost", string("localhost")),
		objectName("objectName", string("GreenCup")),
		collectionName("collectionName", string("containers")),
		nodeTypeProp("nodeType", string("")),//StereoLR, SomXYZRgb, Object
		viewOrModelName("viewOrModelName", string("")),//lab012,
		type("type", string("Object")),	//"View", "Model", ""
		modelType("modelType", string(""))//"SSOM","SOM", ""
//		modelType("modelType", string("SSOM"))
{
		registerProperty(mongoDBHost);
		registerProperty(objectName);
		registerProperty(collectionName);
		registerProperty(nodeTypeProp);
		registerProperty(viewOrModelName);
		registerProperty(modelType);
		registerProperty(type);
        CLOG(LTRACE) << "Hello ObjectRemover";

        //base = new MongoBase::MongoBase();
}

ObjectRemover::~ObjectRemover()
{
        CLOG(LTRACE) << "Good bye ObjectRemover";
}

void ObjectRemover::readfromDB()
{
	CLOG(LNOTICE) << "ObjectRemover::readfromDB";
	string nodeType = (string)nodeTypeProp;
	string viewmodelName = (string)viewOrModelName;
	string typeString = (string)type;
	removeFromMongoDB(nodeType, viewmodelName, typeString);
}
void ObjectRemover::prepareInterface() {
	CLOG(LTRACE) << "ObjectRemover::prepareInterface";
	h_readfromDB.setup(this, &ObjectRemover::readfromDB);
	registerHandler("Remove", &h_readfromDB);
}

bool ObjectRemover::onInit()
{
	CLOG(LTRACE) << "ObjectRemover::initialize";
	if(collectionName=="containers")
		dbCollectionPath="images.containers";

	name_cloud_xyz="";
	name_cloud_xyzrgb="";
	name_cloud_xyzsift="";

	string hostname = mongoDBHost;
	connectToMongoDB(hostname);
	if(collectionName=="containers")
		MongoBase::dbCollectionPath=dbCollectionPath="images.containers";

	return true;
}

bool ObjectRemover::onFinish()
{
        CLOG(LTRACE) << "ObjectRemover::finish";
        return true;
}

bool ObjectRemover::onStep()
{
        CLOG(LTRACE) << "ObjectRemover::step";
        return true;
}

bool ObjectRemover::onStop()
{
        return true;
}

bool ObjectRemover::onStart()
{
        return true;
}

void ObjectRemover::removeFromMongoDB(string& nodeType,  string& modelOrViewName, string& type)
{
	CLOG(LTRACE)<<"ObjectRemover::readFromMongoDB";
	string name;
	std::vector<AbstractObject*> models;
	try{
		int items=0;
		findDocumentInCollection(*c, dbCollectionPath, objectName, nodeType, cursorCollection, modelOrViewName, type, items);
		if(items>0)
		{
			CLOG(LINFO)<<"Founded some data";
			while (cursorCollection->more())
			{
				BSONObj obj = cursorCollection->next();
				BSONObj objTemp = obj;

				vector<OID> childsVector;
				int items =  getChildOIDS(obj, "childOIDs", "childOID", childsVector);
				// if node has a child
				int queryOptions = 0;
				const BSONObj *fieldsToReturn = 0;

				//TODO add remove from objects from scenes
				if(!obj.isEmpty() && nodeTypeProp=="Object")
				{

					CLOG(LFATAL)<<"Type: "<<nodeType;
					CLOG(LFATAL)<<"Object: "<<objTemp;
					//remove document
					BSONElement oi;
					objTemp.getObjectID(oi);
					OID docOID = oi.__oid();

					CLOG(LFATAL)<<"Usuwamy: "<<docOID.toString();
					c->remove(dbCollectionPath , Query(BSON("_id" << docOID)));
					CLOG(LFATAL)<<"usunieto";
				}//if
				//if(nodeType=="SSOM")
			//	{
					CLOG(LFATAL)<<"Type: "<<nodeType;
					CLOG(LFATAL)<<"Object:"<<obj;
		//		}

				if(items>0)
				{
					if(isViewLastLeaf(nodeType) || isModelLastLeaf(nodeType))
					{
						// remove all files from grid or collection included in childVector<OID&> and remove them from actual document

						for(std::vector<OID>::iterator it = childsVector.begin(); it != childsVector.end(); ++it)
						{
							// remove ids from table
							CLOG(LERROR)<<"Remove oids : "<<it->toString() << " from table";
							c->update(dbCollectionPath, Query(BSON("ObjectName"<<objectName<<type+"Name"<<modelOrViewName<<"Type"<<nodeType)), BSON("$pull"<<BSON("childOIDs"<<BSON("childOID"<<it->toString()))), false, true);

							BSONObj objFile = c->findOne(dbCollectionPath, Query(BSON("_id" << *it)), fieldsToReturn, queryOptions);
							string place = objFile.getField("place").str();
							CLOG(LERROR)<<"obj: "<<objFile<<", childOID: "<<*it;
							CLOG(LERROR)<<"place: "<<place;
							if(place=="collection")
							{
								c->remove(dbCollectionPath , Query(BSON("_id" << *it)));
							}
							else
							{
								GridFS fs(*c,collectionName);
								GridFile file = fs.findFile(Query(BSON("_id" << *it)));
								if (!file.exists())
								{
									CLOG(LERROR) << "File not found in grid";
								}
								else
								{
									// get filename
									string filename = file.getFileField("filename").str();
									fs.removeFile(filename);
								}
							}
						}
					}
					else
					{

						CLOG(LTRACE)<<"There are childs "<<childsVector.size();
						for (unsigned int i = 0; i<childsVector.size(); i++)
						{
							childCursor =c->query(dbCollectionPath, (Query(BSON("_id"<<childsVector[i]))));
							if(childCursor->more())
							{
								BSONObj childObj = childCursor->next();
								string childNodeName= childObj.getField("Type").str();
								CLOG(LINFO)<<"childNodeName: "<<childNodeName;
								if(childNodeName!="EOO")
								{
									if(childNodeName=="Model")
									{
										CLOG(LINFO)<<"setModelName";
										string newName;
										setModelOrViewName(childNodeName, childObj, newName);
										type="Model";
										removeFromMongoDB(childNodeName, newName, type);
									}
									else if(childNodeName=="View")
									{
										CLOG(LINFO)<<"setViewName";
										string newName;
										setModelOrViewName(childNodeName, childObj, newName);
										type="View";
										removeFromMongoDB(childNodeName, newName, type);
									}
									else if(childNodeName=="SOM" || childNodeName=="SSOM")
									{
										CLOG(LINFO)<<"nodeTypeProp: "<<nodeTypeProp;
										if(modelType==childNodeName || nodeTypeProp=="Object")
										{
											CLOG(LINFO)<<"modelType==childNodeName || nodeTypeProp==Object";
											removeFromMongoDB(childNodeName, modelOrViewName, type);
										}
									}
									else
										removeFromMongoDB(childNodeName, modelOrViewName, type);
								}
							}//if(childNodeName!="EOO")
						}//for
					}
				}//if
			}//while
		}//if
		else
		{
			CLOG(LTRACE)<<"10";
			if(nodeTypeProp==nodeType)
				CLOG(LERROR)<<"Wrong name";
			CLOG(LTRACE)<<"No results";
		}
		/*
		// stworz model i wyslij do sinka, jesli przegladanie drzewa jest zakonczone
		if(nodeTypeProp==nodeType)
		{
			readAllFilesTriggered();
		}
		*/
	}//try
	catch(DBException &e)
	{
		CLOG(LERROR) <<"RemoveFromMongoDB(). Something goes wrong... :<";
		e.what();
		exit(1);
	}
}
} //: namespace ObjectRemover
} //: namespace Processors