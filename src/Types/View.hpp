/*
 * View.hpp
 *
 *  Created on: Nov 20, 2014
 *      Author: lzmuda
 */

#ifndef VIEW_HPP_
#define VIEW_HPP_

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>

#include "Logger.hpp"

#include <cstdlib>
#include <iostream>
#include <glob.h>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "Logger.hpp"
#include "mongo/client/dbclient.h"
#include "Component_Aux.hpp"
#include "Component.hpp"
#include "DataStream.hpp"
#include "Property.hpp"
#include <dirent.h>
#include <iostream>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/compression/octree_pointcloud_compression.h>

#include <Types/PointXYZSIFT.hpp>
#include <Types/PointXYZRGBSIFT.hpp>
#include <Types/SIFTObjectModelFactory.hpp>
#include <Types/MongoBase.hpp>
#include <Types/PrimitiveFile.hpp>

#include <vector>
#include <list>
#include <iostream>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>

namespace MongoBase{
using namespace cv;
using namespace mongo;
using namespace std;
using namespace boost;
using namespace PrimitiveFile;

class View : public MongoBase::MongoBase
{
private:
	string ViewName;						// lab012
	string SensorType;						// Stereo, ToF...
	string dateOfInsert;					// 02042013
	std::vector<shared_ptr<PrimitiveFile::PrimitiveFile> > files;
	std::vector<string>	fileTypes;			// [MASK, IMAGE, …, IMAGE3D]

	// all required types to store
	std::vector<keyTypes> requiredKeyTypes;

	// inserted file types of file
	std::vector<keyTypes> insertedKeyTypes;

public:
	View(string& viewName) : ViewName(viewName){};
	void setRequiredKeyTypes(std::vector<keyTypes> &);
	void addFile();
	void getAllFiles();
	void saveAllFiles();
	void removeFile();
	void removeAllFiles();
	void setViewName();
	void getViewName();
	void setSensorType();
	void getSensorType();
	void setDateOfInsert();
	bool checkIfExist();
	void getDateOfInsert();
	void create();

	// check if exist this same kind of file
	bool checkIfFileExist(keyTypes key);

	bool checkIfAllFiles();
	void putStringToFile(const std::string& str, keyTypes key);
	void putMatToFile(const cv::Mat& image, keyTypes key);
	void putPCxyzToFile(const pcl::PointCloud<pcl::PointXYZ>::Ptr&, keyTypes key);
	void putPCyxzrgbToFile(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr&, keyTypes key);
	void putPCxyzsiftToFile(const pcl::PointCloud<PointXYZSIFT>::Ptr&, keyTypes key);
	void putPCxyzrgbsiftToFile(const pcl::PointCloud<PointXYZRGBSIFT>::Ptr&, keyTypes key);
	void putPCxyzshotToFile(const pcl::PointCloud<PointXYZSHOT>::Ptr&, keyTypes key);
	void putPCxyzrgbNormalToFile(const pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr&, keyTypes key);

};// class View

// check if all required types of file are present in vector
// if types are stereo, check  all stereo files and stereo textured files if needed

bool View::checkIfAllFiles()
{
	bool present = false;
	bool stereoLPresent = false;
	bool stereoRPresent = false;
	bool stereoLTexturedPresent = false;
	bool stereoRTexturedPresent = false;

	for(std::vector<keyTypes>::iterator reqTypes = requiredKeyTypes.begin(); reqTypes != requiredKeyTypes.end(); ++reqTypes)
	{
		for(std::vector<keyTypes>::iterator insTypes = insertedKeyTypes.begin(); insTypes != insertedKeyTypes.end(); ++insTypes)
		{
			if(*reqTypes==*insTypes)
			{
				LOG(LINFO)<<"Present in files, type: "<< *reqTypes;
				present = true;
				break;
			}
			else if(*insTypes==stereoL)
				stereoLPresent = true;
			else if(*insTypes==stereoR)
				stereoRPresent = true;
			else if(*insTypes==stereoLTextured)
				stereoLTexturedPresent = true;
			else if(*insTypes==stereoRTextured)
				stereoRTexturedPresent = true;
		}// for
		if(present)
		{
			present = false;
		}
		else if(*reqTypes==stereo)
		{
			if(!stereoLPresent || !stereoRPresent)
				return false;
		}
		else if(*reqTypes==stereoTextured)
		{
			if(!stereoLPresent ||  !stereoRPresent || !stereoLTexturedPresent || !stereoRTexturedPresent)
				return false;
		}
		else
			return false;
	}
	return true;
}

bool View::checkIfExist()
{
	return false;
}
// check if in view exist this same kind of file
bool View::checkIfFileExist(keyTypes key)
{
	for(std::vector<boost::shared_ptr<PrimitiveFile::PrimitiveFile> >::iterator it = files.begin(); it != files.end(); ++it)
	{
		if(key==it->get()->getType())
			return true;
	}
	return false;
}

void View::create()
{
	return;
}

void View::putStringToFile(const std::string& str, keyTypes key)
{
	LOG(LNOTICE)<< "View::putStringToFile";
	LOG(LNOTICE)<< "key: "<<key;
	shared_ptr<PrimitiveFile::PrimitiveFile> file(new PrimitiveFile::PrimitiveFile);

	// insert data to file
	// moze przerobic to na konstruktor? Wtedy mógłby być jeden przeciążony konstruktor
	// do wszystkiego :)
	file->putString(str, key);
	// i wszystko co poniżej byłoby już w jednej metodzie...

	// create new method from here

	// add file to vector
	files.push_back(file);
	insertedKeyTypes.push_back(key);

	// check if all required files are present in view
	bool allFiles = checkIfAllFiles();

	if(allFiles)
	{
		LOG(LNOTICE)<<"Write view to data base";
		// saveAllFiles()
	}
	else
	{
		LOG(LNOTICE) << "Waiting for all files to save them in mongoDB";
	}

	// to here

}

void View::putPCxyzToFile(const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloudXYZ, keyTypes key)
{
	LOG(LNOTICE)<< "View::putPCxyzToFile";
	LOG(LNOTICE)<< "key: "<<key;
	shared_ptr<PrimitiveFile::PrimitiveFile> file(new PrimitiveFile::PrimitiveFile);
	file->putPCxyz(cloudXYZ, key);
	// add file to vector
	files.push_back(file);
	insertedKeyTypes.push_back(key);

	// check if all required files are present in view
	bool allFiles = checkIfAllFiles();

	if(allFiles)
	{
		LOG(LNOTICE)<<"Write view to data base";
		// saveAllFiles()
	}
	else
	{
		LOG(LNOTICE) << "Waiting for all files to save them in mongoDB";
	}
}
void View::putPCyxzrgbToFile(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr& cloudXYZRGB, keyTypes key)
{
	LOG(LNOTICE)<< "View::putPCyxzrgbToFile";
	LOG(LNOTICE)<< "key: "<<key;
	shared_ptr<PrimitiveFile::PrimitiveFile> file(new PrimitiveFile::PrimitiveFile);
	file->putPCyxzrgb(cloudXYZRGB, key);
	files.push_back(file);
}
void View::putPCxyzsiftToFile(const pcl::PointCloud<PointXYZSIFT>::Ptr& cloudXYZSIFT, keyTypes key)
{
	LOG(LNOTICE)<< "View::putPCxyzsiftToFile";
	LOG(LNOTICE)<< "key: "<<key;
	shared_ptr<PrimitiveFile::PrimitiveFile> file(new PrimitiveFile::PrimitiveFile);
	file->putPCxyzsift(cloudXYZSIFT, key);
	files.push_back(file);
}

void View::putPCxyzrgbsiftToFile(const pcl::PointCloud<PointXYZRGBSIFT>::Ptr& cloudXYZRGBSIFT, keyTypes key)
{
	LOG(LNOTICE)<< "View::putPCxyzrgbsiftToFile";
	LOG(LNOTICE)<< "key: "<<key;
	shared_ptr<PrimitiveFile::PrimitiveFile> file(new PrimitiveFile::PrimitiveFile);
	file->putPCxyzrgbsift(cloudXYZRGBSIFT, key);
	files.push_back(file);
}

void View::putPCxyzshotToFile(const pcl::PointCloud<PointXYZSHOT>::Ptr& cloudXYZSHOT, keyTypes key)
{
	LOG(LNOTICE)<< "View::putPCxyzshotToFile";
	LOG(LNOTICE)<< "key: "<<key;
	shared_ptr<PrimitiveFile::PrimitiveFile> file(new PrimitiveFile::PrimitiveFile);
	file->putPCxyzshot(cloudXYZSHOT, key);
	files.push_back(file);
}

void View::putPCxyzrgbNormalToFile(const pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr& cloudXYZRGBNormal, keyTypes key)
{
	LOG(LNOTICE)<< "View::putPCxyzrgbNormalToFile";
	LOG(LNOTICE)<< "key: "<<key;
	shared_ptr<PrimitiveFile::PrimitiveFile> file(new PrimitiveFile::PrimitiveFile);
	file->putPCxyzrgbNormal(cloudXYZRGBNormal, key);
	files.push_back(file);
}

void View::putMatToFile(const cv::Mat& img, keyTypes key)
{
	LOG(LNOTICE)<< "View::putMatToFile";
	LOG(LNOTICE)<< "key: "<<key;
	shared_ptr<PrimitiveFile::PrimitiveFile> file(new PrimitiveFile::PrimitiveFile);
	file->putMat(img, key);
	// add file to vector
	files.push_back(file);
	insertedKeyTypes.push_back(key);

	// check if all required files are present in view
	bool allFiles = checkIfAllFiles();

	if(allFiles)
	{
		LOG(LNOTICE)<<"Write view to data base";
		// saveAllFiles()
	}
	else
	{
		LOG(LNOTICE) << "Waiting for all files to save them in mongoDB";
	}
}

}//namespace MongoBase




#endif /* VIEW_HPP_ */