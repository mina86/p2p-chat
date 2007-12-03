/**
 * XML parser tester.
 * $Id: xml-test.cpp,v 1.1 2007/12/03 14:46:33 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>

#include "xml-parser.hpp"

namespace test {

class XMLParser : public ppc::xml::Parser {
protected:
	virtual void open(const std::string &element) {
		printf("open(%s)\n", element.c_str());
	}

	virtual void attribute(const std::string &name,const std::string &value) {
		printf("attribute(%s = %s)\n", name.c_str(), value.c_str());
	}

	virtual void close(const std::string &name) {
		printf("close(%s)\n", name.c_str());
	}

	virtual void cdata(const std::string &data) {
		printf("cdata(%s)\n", data.c_str());
	}
};


}

int main(void) {
	test::XMLParser parser;
	char buffer[128];

	try {
		parser.init();
		while (fgets(buffer, sizeof buffer, stdin)) {
			parser.feed(buffer, strlen(buffer));
		}
		parser.done();
	}
	catch (const ppc::xml::Error &e) {
		printf("error: %s\n", e.getMessage().c_str());
		return 1;
	}

	return 0;
}
