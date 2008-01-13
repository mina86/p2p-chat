/** \file
 * Config structure implementation.
 * $Id: config.cpp,v 1.1 2008/01/13 13:30:54 mina86 Exp $
 */

#include "config.hpp"

#include <iostream>
#include <string>

/** Size of buffer used for reading file */
#define READ_BUFFER_SIZE 1024

namespace ppc {

FILE * Config::openFile(const std::string& fileName, const char *mode){
	return fileName.empty() ? 0 : fopen(fileName.c_str(), mode);
}

int Config::loadConfig(){
	char buffer[READ_BUFFER_SIZE];

	FILE* fd = openFile(configFile, "r");
	if(fd == 0){
		return 1;
	}
	tree->reset();

	size_t readelements;
	while(ferror(fd)==0 && feof(fd)==0){
		readelements = fread(buffer, sizeof(char), READ_BUFFER_SIZE, fd);
		parser->feed(buffer, readelements);
	}
	parser->done();

	if (fclose(fd) == EOF){
		return 2;
	}
	return 0;
}

int Config::saveConfig(const std::string& fileName){
	FILE* fd = openFile(fileName, "w");
	if(fd == 0){
		return 1;
	}
	std::string strout(tree->printTree());
	fwrite(&strout[0], sizeof(char), strout.size(), fd);

	if (fclose(fd) == EOF){
		return 2;
	}
	return 0;
}


}
