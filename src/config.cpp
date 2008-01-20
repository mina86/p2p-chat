/** \file
 * Config structure implementation.
 * $Id: config.cpp,v 1.3 2008/01/20 17:49:26 mina86 Exp $
 */

#include <errno.h>
#include <iostream>
#include <string>

#include "config.hpp"
#include "shared-buffer.hpp"


namespace ppc {

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
		if (ferror(fd) || feof(fd)) {
			break;
		}
		reader.feed(sharedBuffer, readelements);
	}
	reader.done();

	return fclose(fd) == EOF ? 2 : 0;
}

int ConfigFile::saveConfig(const std::string& fileName){
	FILE* fd = openFile(fileName, "w");
	if(fd == 0){
		return 1;
	}

	getRoot()->printNode(fd);

	return fclose(fd) == EOF ? 2 : 0;
}

const std::string& Config::getString(const std::string &path,
                                     const std::string &def) const {
	std::string::size_type index = path.find(':');
	xml::ElementNode *node =
		root->findNode(index == std::string::npos
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


}
