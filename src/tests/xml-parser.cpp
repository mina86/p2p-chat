/** \file
 * XML & PPCP parser tester.
 * Copyright 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
