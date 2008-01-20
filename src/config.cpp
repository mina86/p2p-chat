/** \file
 * Config structure implementation.
 * $Id: config.cpp,v 1.2 2008/01/20 16:24:43 jwawer Exp $
 */

#include "config.hpp"

#include <iostream>
#include <string>

/** Size of buffer used for reading file */
#define READ_BUFFER_SIZE 1024

namespace ppc {

FILE * ConfigFile::openFile(const std::string& fileName, const char *mode){
	return fileName.empty() ? 0 : fopen(fileName.c_str(), mode);
}

int ConfigFile::loadConfig(const std::string& fileName) {
	char buffer[READ_BUFFER_SIZE];
	xml::Reader reader(getRoot());
	
	configFile = fileName;
	FILE* fd = openFile(configFile, "r");
	if(fd == 0){
		return 1;
	}

	while(ferror(fd)==0 && feof(fd)==0){
		size_t readelements = fread(buffer, sizeof(char), READ_BUFFER_SIZE, fd);
		reader.feed(buffer, readelements);
	}
	reader.done();

	if (fclose(fd) == EOF){
		return 2;
	}
	return 0;		
}

int ConfigFile::saveConfig(const std::string& fileName){
	FILE* fd = openFile(fileName, "w");
	if(fd == 0){
		return 1;
	}
	
	getRoot()->printNode(fd);

	if (fclose(fd) == EOF){
		return 2;
	}
	return 0;
}

const std::string& Config::getString(const std::string &path, 
		const std::string &def) const {

	size_t index = path.find_first_of(":");
	std::string nodePath = path.substr(0, index);
	std::string attr;
	if(index != std::string::npos){
		attr = path.substr(index+1);
	}	

	xml::ElementNode *node = root->findNode(nodePath);
	if (node == 0){
		return def;
	}
	if ( attr.empty() ){
		xml::CDataNode *cnode = node->findCData();
		if (cnode == 0){
			return def;
		}
		return cnode->getCData();
	}
	else{
		return node->getAttr(attr, def);
	}
}

unsigned long  Config::getUnsigned(const std::string &path, unsigned long def) const {
	std::string str = getString(path);
	if (str.empty()){
		return def;
	}

	errno = 0;		
	unsigned long res = strtoul(str.c_str(), 0, 10);
	if (errno){
		return def;
	}
	return res;
};

long  Config::getInteger(const std::string &path, long def) const {
	std::string str = getString(path);
	if (str.empty()){
		return def;
	}

	errno = 0;		
	long res = strtol(str.c_str(), 0, 10);
	if (errno){
		return def;
	}
	return res;	
};

double  Config::getReal(const std::string &path, double def) const {	
	std::string str = getString(path);
	if (str.empty()){
		return def;
	}

	errno = 0;		
	double res = strtod(str.c_str(), 0);
	if (errno){
		return def;
	}
	return res;	
};


}
