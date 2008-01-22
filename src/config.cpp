/** \file
 * Config structure implementation.
 * $Id: config.cpp,v 1.8 2008/01/22 20:56:34 jwawer Exp $
 */

#include <errno.h>
#include <iostream>
#include <string>

#include "config.hpp"
#include "shared-buffer.hpp"


namespace ppc {


/************************ Config methods ************************/

/* Methods to get config values */

const std::string& Config::getString(const std::string &path,
                                     const std::string &def) const {
	std::string::size_type index = path.find('@');
	xml::ElementNode *node =
		root.findNode(index == std::string::npos
		              ? path : std::string(path, 0, index));
	if (!node) {
		return def;
	} else if (index == std::string::npos) {
		xml::CDataNode *cnode = node->findCData();
		return cnode ? cnode->getCData() : def;
	} else {
		return node->getAttr(std::string(path, index + 1), def);
	}
}

unsigned long Config::getUnsigned(const std::string &path,
                                  unsigned long def) const {
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
}

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
}

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
}

/* Methods to set config values */

void Config::setString(const std::string &path, const std::string &val) {
	size_t index = path.find_first_of("@");
	std::string nodePath = path.substr(0, index);
	std::string attr;
	if(index != std::string::npos){
		attr = path.substr(index+1);
	}

	xml::ElementNode *node = root.modifyNode(nodePath);

	if (attr.empty()) {
		node->modifyCData(val);
	} else {
		node->setAttr(attr, val);
	}
}

void Config::setUnsigned(const std::string &path, unsigned long val) {
	sprintf(sharedBuffer, "%lu", val);
	setString(path, sharedBuffer);
}

void Config::setInteger(const std::string &path, long val) {
	sprintf(sharedBuffer, "%ld", val);
	setString(path, sharedBuffer);
}

void Config::setReal(const std::string &path, double val) {
	sprintf(sharedBuffer, "%f", val);
	setString(path, sharedBuffer);
}


/********************** ConfigFile methods **********************/

/**
 * Simple function used for maintaining opening files.
 *
 * \param fileName name of file to open.
 * \param mode mode used to opening file (r(ead) or w(rite)).
 * \return opened file descriptor or 0 if there was any problem
 */
static FILE *openFile(const std::string& fileName, const char *mode){
	return fileName.empty() ? 0 : fopen(fileName.c_str(), mode);
}

int ConfigFile::loadConfig(const std::string& fileName) {
	xml::Reader reader(getRoot());

	configFile = fileName;
	FILE* fd = openFile(configFile, "r");
	if(fd == 0){
		return 1;
	}

	for(;;) {
		size_t readelements = fread(sharedBuffer, 1, sizeof sharedBuffer, fd);
		reader.feed(sharedBuffer, readelements);
		if (ferror(fd) || feof(fd)) {
			break;
		}
	}
	reader.done();

	return fclose(fd) == EOF ? 2 : 0;
}

int ConfigFile::saveConfig(const std::string& fileName){
	FILE* fd = openFile(fileName, "w");
	if(fd == 0){
		return 1;
	}

	if(getRoot().getFirstChild()){
		getRoot().getFirstChild()->printNode(fd);
	}
	return fclose(fd) == EOF ? 2 : 0;
}


}
