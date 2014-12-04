/*!
 * \file
 * \brief
 * \author Lukasz Zmuda
 */
#include "ModelReader.hpp"

#define siftPointSize 133
#define xyzPointSize 3
#define xyzrgbPointSize 4

namespace Processors {
namespace ModelReader  {
using namespace cv;
using namespace mongo;
using namespace boost::property_tree;

ModelReader::ModelReader(const std::string & name) : Base::Component(name),
	mongoDBHost("mongoDBHost", string("localhost")),
	modelName("ModelName", string("model1")),
	pc_xyzProp("PC.xyz", false),
	pc_xyzrgbProp("PC.xyzrgb", false),
	pc_xyzsiftProp("PC.xyzsift", false),
	pc_xyzrgbsiftProp("PC.xyzrgbsift", false),
	pc_xyzshotProp("PC.xyzshot", false),
	pc_xyzrgbnormalProp("PC.xyzrgbnormal", false)
{
	registerProperty(mongoDBHost);
	registerProperty(modelName);
	registerProperty(pc_xyzProp);
	registerProperty(pc_xyzrgbProp);
	registerProperty(pc_xyzsiftProp);
	registerProperty(pc_xyzrgbsiftProp);
	registerProperty(pc_xyzshotProp);
	registerProperty(pc_xyzrgbnormalProp);
	hostname = mongoDBHost;
	CLOG(LTRACE) << "Hello ModelReader";
}

ModelReader::~ModelReader()
{
        CLOG(LTRACE) << "Good bye ModelReader";
}


void ModelReader::readfromDB()
{
	CLOG(LNOTICE) << "ModelReader::readfromDB";
	MongoProxy::MongoProxy::getSingleton(hostname);
	string vn = string(modelName);
	modelPtr = boost::shared_ptr<Model>(new Model(vn,hostname));

	bool exist = modelPtr->checkIfExist();
	if(!exist)
	{
		CLOG(LERROR)<<"View doesn't exist in data base!!!, Change view name";
		return;
	}
	else
	{
		// get view document
		modelPtr->readModelDocument();

		// read all required types from GUI
		std::vector<fileTypes> requiredFileTypes;
		readRequiredData(requiredFileTypes);


		if(requiredFileTypes.size()==0)
		{
			CLOG(LERROR)<<"Please mark any checkbox";
			return;
		}
		// check if view contain all required types
		bool contain = modelPtr->checkIfContain(requiredFileTypes);

		if(!contain)
		{
			CLOG(LERROR)<<"View doesn't contain all required files! BYE!";
		}
		else
		{
			CLOG(LNOTICE)<<"Read files from View!";

			// read vector of files OIDs
			vector<OID> fileOIDSVector;
			modelPtr->getAllFilesOIDS(fileOIDSVector);

			// for full required files vector, read file document and check if its type is equal
			// one of requested file types

			modelPtr->readFiles(fileOIDSVector, requiredFileTypes);

			//write to output
			LOG(LNOTICE)<<"WRITE FILE!!!";
			int filesNr = modelPtr->getFilesSize();
			for (int i=0; i<filesNr; i++)
			{
				// get type
				fileTypes ft = modelPtr->getFileType(i);
				// send to output
				switch(ft)
				{
					case PCXyz:
					{
						pcl::PointCloud<pcl::PointXYZ>::Ptr cloudXYZ;
						modelPtr->getFile(i)->getXYZData(cloudXYZ);
						out_pc_xyz.write(cloudXYZ);
						break;
					}
					case PCXyzRgb:
					{
						pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloudXYZRGB;
						modelPtr->getFile(i)->getXYZRGBData(cloudXYZRGB);
						out_pc_xyzrgb.write(cloudXYZRGB);
						break;
					}
					case PCXyzSift:
					{
						pcl::PointCloud<PointXYZSIFT>::Ptr cloudXYZSIFT;
						modelPtr->getFile(i)->getXYZSIFTData(cloudXYZSIFT);
						out_pc_xyzsift.write(cloudXYZSIFT);
						break;
					}
					case PCXyzRgbSift:
					{
						pcl::PointCloud<PointXYZRGBSIFT>::Ptr cloudXYZRGBSIFT;
						modelPtr->getFile(i)->getXYZRGBSIFTData(cloudXYZRGBSIFT);
						out_pc_xyzrgbsift.write(cloudXYZRGBSIFT);
						break;
					}
					case PCXyzShot:
					{
						pcl::PointCloud<PointXYZSHOT>::Ptr cloudXYZSHOT;
						modelPtr->getFile(i)->getXYZSHOTData(cloudXYZSHOT);
						out_pc_xyzshot.write(cloudXYZSHOT);
						break;
					}
					case PCXyzRgbNormal:
					{
						pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloudXYZNormal;
						modelPtr->getFile(i)->getXYZRGBNormalData(cloudXYZNormal);
						out_pc_xyzrgbnormal.write(cloudXYZNormal);
						break;
					}
				}
			}

		}
	}

}

void ModelReader::readRequiredData(std::vector<fileTypes> & requiredFileTypes)
{
	CLOG(LNOTICE)<<"ViewWriter::checkProvidedData";
	bool cleanBuffers = false;

	if(pc_xyzProp==true)
	{
		requiredFileTypes.push_back(PCXyz);
	}
	if(pc_xyzrgbProp==true)
	{
		requiredFileTypes.push_back(PCXyzRgb);
	}
	if(pc_xyzsiftProp==true)
	{
		requiredFileTypes.push_back(PCXyzSift);
	}
	if(pc_xyzrgbsiftProp==true)
	{
		requiredFileTypes.push_back(PCXyzRgbSift);
	}
	if(pc_xyzshotProp==true)
	{
		requiredFileTypes.push_back(PCXyzShot);
	}
	if(pc_xyzrgbnormalProp==true)
	{
		requiredFileTypes.push_back(PCXyzRgbNormal);
	}
	CLOG(LNOTICE)<<"Size of required file types: "<<requiredFileTypes.size();
}

void ModelReader::prepareInterface() {
	CLOG(LTRACE) << "ModelReader::prepareInterface";
	h_readfromDB.setup(this, &ModelReader::readfromDB);
	registerHandler("Read", &h_readfromDB);
	registerStream("out_pc_xyz", &out_pc_xyz);
	registerStream("out_pc_xyzrgb", &out_pc_xyzrgb);
	registerStream("out_pc_xyzsift", &out_pc_xyzsift);
	registerStream("out_pc_xyzrgbsift", &out_pc_xyzrgbsift);
	registerStream("out_pc_xyzshot", &out_pc_xyzshot);
	registerStream("out_pc_xyzrgbnormal", &out_pc_xyzrgbnormal);
}

bool ModelReader::onInit()
{
	CLOG(LTRACE) << "ModelReader::initialize";


	return true;
}

bool ModelReader::onFinish()
{
        CLOG(LTRACE) << "ModelReader::finish";
        return true;
}

bool ModelReader::onStep()
{
        CLOG(LTRACE) << "ModelReader::step";
        return true;
}

bool ModelReader::onStop()
{
        return true;
}

bool ModelReader::onStart()
{
        return true;
}

void ModelReader::addToAllChilds(std::vector<OID> & childsVector)
{
	CLOG(LTRACE)<<"ModelReader::addToAllChilds";
}
/*
void ModelReader::readAllFilesTriggered()
{

	CLOG(LTRACE)<<"ModelReader::readAllFilesTriggered";
	std::vector<AbstractObject*> models;
	for(std::vector<OID>::iterator it = allChildsVector.begin(); it != allChildsVector.end(); ++it)
	{
		//przerobic zeby czytalo tez bezposrednio z dokumentow...
		readFile(*it, models);
	}
	CLOG(LTRACE)<<"Send models to sink";
	out_cloud_xyzrgb.write(cloud_xyzrgb);
	out_models.write(models);
}


void ModelReader::loadModels(string& name_cloud, string& features_number, std::vector<AbstractObject*>& models) {
	CLOG(LTRACE) << "SOMJSONReader::loadModels()";

	model_name = nodeNameProp;
	if(name_cloud.find("xyzrgb")!=string::npos)
		name_cloud_xyzrgb = name_cloud;
	else if(name_cloud.find("xyzsift")!=string::npos)
	{
		istringstream (features_number) >> mean_viewpoint_features_number;
		name_cloud_xyzsift = name_cloud;
	}
	else if(name_cloud.find("xyz")!=string::npos)
		name_cloud_xyz = name_cloud;

	CLOG(LDEBUG) << "name_cloud_xyzrgb:" << name_cloud_xyzrgb;
	CLOG(LDEBUG) << "name_cloud_xyzsift:" << name_cloud_xyzsift;
	CLOG(LDEBUG) << "name_cloud_xyz:" << name_cloud_xyz;

	// Create SOModel and add it to list.
	if(name_cloud_xyzsift!="" && name_cloud_xyzrgb!="")
	{
		CLOG(LDEBUG) << "Create model";
		SIFTObjectModel* model;
		//TODO sprawdzic czy przypadkiem nie trzeba tworzyc modeli typu SSOM, to generuje tylko SOM
		model = dynamic_cast<SIFTObjectModel*>(produce());
		models.push_back(model);
		name_cloud_xyzsift="";
		name_cloud_xyzrgb="";
	}
}

vo
*/
} //: namespace ModelReader
} //: namespace Processors
