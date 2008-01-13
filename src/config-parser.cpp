/** \file
 * Config parser implementation.
 * $Id: config-parser.cpp,v 1.1 2008/01/13 13:30:54 mina86 Exp $
 */

#include "config-parser.hpp"

namespace ppc {

void ConfigParser::open(const std::string &name, const Attributes &attrs){
	tree->openNode(name, attrs);
}

void ConfigParser::cdata(const std::string &data){
	std::string dirt("\n\t ");
	size_t start = data.find_first_not_of(dirt);
	if (start == std::string::npos){
		return;
	}
	size_t lenght = data.find_last_not_of(dirt) - start +1;
	std::string cleanData = data.substr(start, lenght);
	tree->fillNode(cleanData);
}

void ConfigParser::close(const std::string &name){
	(void)name;
	tree->closeNode();
}

}

