/** \file
 * XML & PPCP parser tester.
 * $Id: xml-parser.cpp,v 1.2 2007/12/10 12:35:07 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>

#include <map>

#include "../xml-parser.hpp"
#include "../ppcp-parser.hpp"

namespace test {

class XMLParser : public ppc::xml::TokenConsumer {
protected:
	virtual void open(const std::string &element) {
		printf("EV %16s: %s\n", "open", element.c_str());
	}

	virtual void attribute(const std::string &name,const std::string &value) {
		printf("EV %16s: %s = %s\n", "attribute", name.c_str(),
		       value.c_str());
	}

	virtual void close(const std::string &name) {
		printf("EV %16s: %s\n", "close", name.c_str());
	}

	virtual void cdata(const std::string &data) {
		printf("EV %16s: '%s'\n", "text", data.c_str());
	}
};


}

/** An XML parser test. */
int main(int argc, char **argv) {
	std::map<unsigned, const char *> xTokenNames;
	std::map<unsigned, const char *> pTokenNames;;
	ppc::xml::Tokenizer tokenizer;
	test::XMLParser parser;
	ppc::ppcp::Tokenizer pTokenizer(argc < 2 ? "mina86" : argv[1], argc > 2);
	char buffer[128];

	xTokenNames.insert(std::make_pair(ppc::xml::Tokenizer::END, "END"));
	xTokenNames.insert(std::make_pair(ppc::xml::Tokenizer::TAG_OPEN,
	                                  "TAG_OPEN"));
	xTokenNames.insert(std::make_pair(ppc::xml::Tokenizer::ATTR_NAME,
	                                  "ATTR_NAME"));
	xTokenNames.insert(std::make_pair(ppc::xml::Tokenizer::ATTR_VALUE,
	                                  "ATTR_VALUE"));
	xTokenNames.insert(std::make_pair(ppc::xml::Tokenizer::TAG_CLOSE,
	                                  "TAG_CLOSE"));
	xTokenNames.insert(std::make_pair(ppc::xml::Tokenizer::TEXT, "TEXT"));
	xTokenNames.insert(std::make_pair(ppc::xml::Tokenizer::ELEMENT_CLOSE,
	                                  "ELEMENT_CLOSE"));

	pTokenNames.insert(std::make_pair(ppc::ppcp::Tokenizer::END, "END"));
	pTokenNames.insert(std::make_pair(ppc::ppcp::Tokenizer::IGNORE,
	                                  "IGNORE"));
	pTokenNames.insert(std::make_pair(ppc::ppcp::Tokenizer::PPCP_OPEN,
	                                  "PPCP_OPEN"));
	pTokenNames.insert(std::make_pair(ppc::ppcp::Tokenizer::ST, "ST"));
	pTokenNames.insert(std::make_pair(ppc::ppcp::Tokenizer::RQ, "RQ"));
	pTokenNames.insert(std::make_pair(ppc::ppcp::Tokenizer::M, "M"));
	pTokenNames.insert(std::make_pair(ppc::ppcp::Tokenizer::PPCP_CLOSE,
	                                  "PPCP_CLOSE"));

	try {
		tokenizer.init();
		pTokenizer.init();
		while (fgets(buffer, sizeof buffer, stdin)) {
			ppc::xml::Tokenizer::Token xToken;
			ppc::ppcp::Tokenizer::Token pToken;

			tokenizer.feed(buffer, strlen(buffer));
			do {
				xToken = tokenizer.nextToken();
				if (xToken) {
					printf("XML %15s: '%s'\n", xTokenNames[xToken.type],
					       xToken.data.c_str());
				}
				parser.consumeToken(xToken);
				pToken = pTokenizer.nextToken(xToken);
				if (pToken) {
					printf("PPCP %14s: '%s' (%u)\n",
					       pTokenNames[pToken.type], pToken.data.c_str(),
					       pToken.flags);
				}
			} while (xToken);
		}
		tokenizer.done();
	}
	catch (const ppc::xml::Error &e) {
		printf("error: %s\n", e.getMessage().c_str());
		return 1;
	}

	return 0;
}
