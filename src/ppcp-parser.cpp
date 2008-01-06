/** \file
 * PPCP parser implementation.
 * $Id: ppcp-parser.cpp,v 1.10 2008/01/06 15:25:41 mina86 Exp $
 */

#include <assert.h>

#include <errno.h>
#include <stdlib.h>

#include "ppcp-parser.hpp"


namespace ppc {

namespace ppcp {


/** Some constants - possible elements, attributes and flags. */
enum {
	E_START           = 0,
	E_PPCP            = 1,
	E_ST              = 2,
	E_RQ              = 3,
	E_M               = 4,
	E_IGNORE          = 255,

	A_PPCP_N          = 1,
	A_PPCP_P          = 2,
	A_PPCP_TO_N       = 3,
	A_PPCP_TO_NEG     = 4,
	A_ST_ST           = 5,
	A_ST_DN           = 6,
	A_M_MSG           = 7,
	A_M_AC            = 8,

	F_PPCP_P          = 1,
	F_PPCP_TO_N       = 2,
	F_PPCP_TO_N_OK    = 4,
	F_PPCP_TO_NEG     = 8
};




void Tokenizer::init() {
	element = E_START;
	ignore = 0;
	data.clear();
	data2.clear();
}



Tokenizer::Token Tokenizer::nextToken(const xml::Tokenizer::Token &xToken) {
	if (element==E_IGNORE) {
		return Token(IGNORE);
	}


	Token token;
	if (!xToken) {
		return token;
	}


	/* Are we ignoring? */
	if (ignore) {
		if (xToken.type == xml::Tokenizer::TAG_OPEN) {
			++ignore;
		} else if (xToken.type == xml::Tokenizer::ELEMENT_CLOSE) {
			--ignore;
		}
		return token;
	}


	/* Do the job */
	switch (xToken.type) {
		/* Tag is opened */
	case xml::Tokenizer::TAG_OPEN:
		attribute = 0;
		flags = 0;
		data.clear();
		data2.clear();
		switch (element) {
		case E_START:
			if (xToken.data != "ppcp") goto ignore_rest;
			element = E_PPCP;
			break;

		case E_PPCP:
			if (xToken.data == "st") {
				flags = (unsigned short)User::ONLINE;
				element = E_ST;
			} else if (xToken.data == "rq") element = E_RQ;
			else if (xToken.data == "m") element = E_M;
			else {
		case E_ST:
		case E_RQ:
		case E_M: ignore = 1;
			}
			break;

		default:
			assert(0);
		}
		break;


	case xml::Tokenizer::END: /* dead code, we have handled it ealier */
		assert(0);
		break;


		/* Attribute */
	case xml::Tokenizer::ATTR_NAME:
		/* Change state depending on attribute name */
		switch (element) {
		case E_PPCP:
			if (xToken.data == "n") attribute = A_PPCP_N;
			if (xToken.data == "p") attribute = A_PPCP_P;
			else if (xToken.data == "to:n") attribute = A_PPCP_TO_N;
			else if (xToken.data == "to:neg") attribute = A_PPCP_TO_NEG;
			break;

		case E_ST:
			if (xToken.data == "st") attribute = A_ST_ST;
			else if (xToken.data == "dn") attribute = A_ST_DN;
			break;

		case E_M:
			if (xToken.data == "msg") attribute = A_M_MSG;
			else if (xToken.data == "ac") attribute = A_M_AC;
			break;

		default:
			assert(0);
		}
		break;


		/* Attribute's value */
	case xml::Tokenizer::ATTR_VALUE:
		switch (attribute) {
		case A_PPCP_N:
			data = xToken.data;
			break;

		case A_PPCP_P:
		case A_ST_DN:
			data2 = xToken.data;
			break;

		case A_PPCP_TO_N:
			flags = (flags & ~F_PPCP_TO_N_OK) | F_PPCP_TO_N;
			if (data == ourNick) flags |= F_PPCP_TO_N_OK;
			break;

		case A_PPCP_TO_NEG:
			flags &= ~F_PPCP_TO_NEG;
			if (data == "neg") flags |= F_PPCP_TO_NEG;
			break;

		case A_ST_ST:
			if (xToken.data == "off") flags = (unsigned)User::OFFLINE;
			else if (xToken.data == "on") flags = (unsigned)User::ONLINE;
			else if (xToken.data == "away") flags = (unsigned)User::AWAY;
			else if (xToken.data == "xa") flags = (unsigned)User::XAWAY;
			else if (xToken.data == "dnd") flags = (unsigned)User::BUSY;
			break;

		case A_M_MSG:
			if (xToken.data == "msg") flags |= Token::M_MESSAGE;
			break;

		case A_M_AC:
			if (xToken.data == "ac") flags |= Token::M_ACTION;
			break;

		default:
			assert(0);
		}
		break;


		/* Opening tag is closed */
	case xml::Tokenizer::TAG_CLOSE:
		if (element != E_PPCP) {
			break;
		}

		if ((flags & F_PPCP_TO_N &&
		     (!(flags & F_PPCP_TO_N_OK) == !(flags & F_PPCP_TO_NEG))) ||
		    data2.empty() || !User::isValidName(data)) {
			goto ignore_rest;
		}

		{
			unsigned long port;
			char *end;
			errno = 0;
			port = strtoul(data2.c_str(), &end, 10);
			if (errno || port<=1024 || port>65535 || *end) goto ignore_rest;
			token.flags = port;
		}

		data = User::nickFromName(data2 = data);
		if (ourPort.host() == token.flags && data == ourNick) {
		ignore_rest:
			token.type = IGNORE;
			element = E_IGNORE;
			data2.clear();
		} else {
			token.type = PPCP_OPEN;
			token.data = data;
			token.data2 = data2;
		}
		data.clear();
		break;


		/* Text */
	case xml::Tokenizer::TEXT:
		if (element == E_ST || element == E_M) {
			data = xToken.data;
		}
		break;


		/* Element is closed */
	case xml::Tokenizer::ELEMENT_CLOSE:
		switch (element) {
		case E_PPCP:
			token.type = PPCP_CLOSE;
			element = E_IGNORE;
			break;

		case E_ST:
			token.type = ST;
			token.data = data;
			token.data2 = data2;
			token.flags = flags;
			element = E_PPCP;
			break;

		case E_RQ:
			token.type = RQ;
			element = E_PPCP;
			break;

		case E_M:
			token.type = M;
			token.data = data;
			token.flags = flags;
			element = E_PPCP;
			break;

		default:
			assert(0);
		}
		break;


	default:
		assert(0);
	}

	return token;
}



Tokenizer::Token Tokenizer::nextToken(xml::Tokenizer &tokenizer) {
	if (element == E_IGNORE) {
		while (tokenizer.nextToken());
		return Token(IGNORE);
	} else {
		xml::Tokenizer::Token xToken;
		Token pToken;
		while ((xToken = tokenizer.nextToken()) &&
		       !(pToken = nextToken(xToken)));
		return pToken;
	}
}


}

}
