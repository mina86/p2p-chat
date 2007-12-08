/** \file
 * XML parser tester.
 * $Id: xml-parser.cpp,v 1.1 2007/12/08 18:02:11 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>

#include <map>

#include "../xml-parser.hpp"

namespace test {

class XMLParser : public ppc::xml::TokenConsumer {
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

/** An XML parser test. */
int main() {
	std::map<unsigned, const char *> tokenNames;
	ppc::xml::Tokenizer tokenizer;
	test::XMLParser parser;
	char buffer[128];

	tokenNames.insert(std::make_pair(ppc::xml::Tokenizer::END, "END"));
	tokenNames.insert(std::make_pair(ppc::xml::Tokenizer::TAG_OPEN,
	                                 "TAG_OPEN"));
	tokenNames.insert(std::make_pair(ppc::xml::Tokenizer::ATTR_NAME,
	                                 "ATTR_NAME"));
	tokenNames.insert(std::make_pair(ppc::xml::Tokenizer::ATTR_VALUE,
	                                 "ATTR_VALUE"));
	tokenNames.insert(std::make_pair(ppc::xml::Tokenizer::TAG_CLOSE,
	                                 "TAG_CLOSE"));
	tokenNames.insert(std::make_pair(ppc::xml::Tokenizer::TEXT,
	                                 "TEXT"));
	tokenNames.insert(std::make_pair(ppc::xml::Tokenizer::ELEMENT_CLOSE,
	                                 "ELEMENT_CLOSE"));

	try {
		tokenizer.init();
		while (fgets(buffer, sizeof buffer, stdin)) {
			ppc::xml::Tokenizer::Token token;
			tokenizer.feed(buffer, strlen(buffer));
			do {
				token = tokenizer.nextToken();
				printf("token %15s: '%s'\n", tokenNames[token.type],
				       token.data.c_str());
			} while (parser.consumeToken(token));
		}
		tokenizer.done();
	}
	catch (const ppc::xml::Error &e) {
		printf("error: %s\n", e.getMessage().c_str());
		return 1;
	}

	return 0;
}
