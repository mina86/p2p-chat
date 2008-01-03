/** \file
 * XML parser implementation.
 * $Id: xml-parser.cpp,v 1.7 2008/01/03 03:00:07 mina86 Exp $
 */

#include <assert.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xml-parser.hpp"
#include "shared-buffer.hpp"


namespace ppc {

namespace xml {


/**
 * Saves unicode charcter in UTF-8 encoding.  Returns pointer to byte
 * after encoded character.
 *
 * \param wr    pointer to buffer to write encoded character to.
 * \param value unicode character to write (must by less then 2**32).
 * \return pointer to buffer after encoded character.
 */
static char *writeUTF8(char *wr, unsigned long value) {
	char *ch = sharedBuffer + sizeof sharedBuffer;

	while (value & ~(0x7fUL)) {
		*--ch = (value & 0x7f) | 0x80;
		value >>= 7;
	}
	if (value >= (0x80UL >> (sharedBuffer + sizeof(sharedBuffer) - ch))) {
		*--ch = (value & 0x7f) | 0x80;
		value >>= 7;
	}
	*wr++ = value | (0xff00 >> (sharedBuffer - ch));

	while (ch!=sharedBuffer + sizeof sharedBuffer) {
		*wr++ = *ch++;
	}

	return wr;
}



std::string &unescapeInPlace(std::string &str) {
	/*
	 * This method is based on observation that character entity
	 * encodes occupies less bytes then entity itself.  If this ever
	 * changed method would have to be rewritten.
	 */

	const char *rd = (char*)memchr(&str[0], '&', str.length());

	if (!rd) {
		return str;
	}

	const char *const end = &str[0] + str.length();
	char *wr = (char*)rd;
	do {
		const char *stop = (char*)memchr(rd, ';', end - rd);
		if (!stop) {
			throw Error("Expecting ';'.");
		}
		++rd;

		/* rd points at first char after '&', stop points at ';' */
		if (*rd=='#') {
			unsigned long value;
			char *ch;
			errno = 0;
			value = rd[1]=='x'?strtoul(rd+2, &ch, 16):strtoul(rd+1, &ch, 10);
			if (errno || ch!=stop || value > 0xffffffff) goto invalid;
			wr = writeUTF8(wr, value);

		} else if (stop-rd == 2 && rd[1]=='t') {
			switch (rd[0]) {
			case 'g': *wr++ = '>'; break;
			case 'l': *wr++ = '<'; break;
			default : goto invalid;
			}

		} else if (stop-rd == 3 && rd[0]=='a' && rd[1]=='m' && rd[2]=='p') {
			*wr++ = '&';

		} else {
		invalid:
			throw Error("Invalid entity name: '"+std::string(rd, stop)+"'");
		}

		rd = stop + 1;
		if (rd==end) break;

		if (!(stop = (char*)memchr(rd, '&', end - rd))) {
			stop = end;
		}
		memmove(wr, rd, stop - rd);
		wr += stop - rd;
		rd = stop;
	} while (rd!=end);

	str.resize(wr - str.data());
	return str;
}



std::string escape(const std::string &str) {
	std::string::size_type count = 0;
	const char *rd = str.data(), *const end = rd + str.length();

	for (; rd!=end; ++rd) {
		/* Yes, 6 as an argument is not a mistake.  We search for
		   NUL bytes as well. */
		if (memchr("<>&\"'", *rd, 6)) {
			++count;
		}
	}

	if (!count) {
		return str;
	}

	/* OMG....  C++ sucks so much...  Why do I need to create a string
	   and fill it with NUL bytes if I'm going to overwrite it
	   anyway?!?!? What about a constructor which allocates memory and
	   leaves random content inside? */

	std::string result(4*count + str.length(), '\0');
	char *wr = &result[0];
	rd = str.data();

	do {
		const char *it = rd;
		while (it != end && !memchr("<>&\"'", *it, 6)) ++it;
		if (it!=rd) {
			memcpy(wr, rd, it - rd);
			wr += it - rd;
		}

		if (it==end) break;
		wr += sprintf(wr, "&#%d;", (int)*it);
		rd = it + 1;

	} while (rd!=end);

	/* In case there were NUL bytes which are converted into 4-byte
	   entity not 5-byte as other special characters. */
	result.resize(wr - result.data());
	return result;
}


/** Possible states. */
enum State {
	START,          /**< We're starting */
	CDATA,          /**< Inside cdata */
	TAG,            /**< Inside tag, reading element name */
	TAG_INSIDE,     /**< Inside open tag, waiting for attribute or end */
	TAG_CLOSING,    /**< Inside close tag, waiting for '>' */
	ATTR,           /**< Reading attribute name */
	ATTR_GOT_NAME,  /**< Got attribute name, waiting for '=' */
	ATTR_GOT_EQ,    /**< Got arg name and '=', waiting for '"' */
	ATTR_RD_VALUE   /**< Reading attribute value, waiting for '"' */
};


Tokenizer::Token Tokenizer::nextToken() {
	std::string::size_type p;
	Token token;

	if (pos >= buffer.length()) {
		return token;
	}

	const char *const data = buffer.data();


	switch ((enum State)state) {
		/* We're starting. */
	case START:
		p = buffer.find_first_not_of(" \t\n\f\v", pos);
		if (p == std::string::npos) {
			buffer.clear();
			pos = 0;
			break;
		}

		if (data[p] != '<') {
			throw Error("Expecting root element.");
		}
		goto state_tag;


		/* Inside CDATA, */
	case CDATA:
		p = buffer.find_first_of("<>", pos);
		if (p == std::string::npos) {
			goto shorten_buffer;
		}

		if (data[p] == '>') {
			throw Error("Unexpected '>'.");
		}

		if (p != dataStart) {
			token.type = TEXT;
			token.data.assign(data + dataStart, p - dataStart);
			state = TAG;
			dataStart = pos = p + 1;
			break;
		}

		/* FALL THROU */


		/* Just after '<', reading element name */
	state_tag:
		state = TAG;
		dataStart = pos = p + 1;
	case TAG:
		const char *it = data + pos, *const end = data + buffer.length();
		if (dataStart == pos && it!=end && *it == '/') ++it;
		while (it != end && isNameChar(*it)) ++it;
		if (it==end) {
			goto shorten_buffer;
		}

		pos = it - data;
		if (pos==dataStart || (pos-dataStart==1 && data[dataStart]=='/')) {
			throw Error("Expecting element name.");
		}

		/* It's opening tag */
		if (data[dataStart] != '/') {
			token.type = TAG_OPEN;
			token.data.assign(data + dataStart, pos - dataStart);
			state = TAG_INSIDE;
			stack.push_back(token.data);
			break;
		}

		/* It's a closing tag */
		std::string name(data + dataStart + 1, pos - dataStart - 1);

		if (stack.empty()) {
			throw Error("Closing '" + name + "' where no element open.");
		} else if (stack.back() != name) {
			throw Error("Closing '"+name+"' where '"+stack.back()+"' open.");
		}

		state = TAG_CLOSING;
		/* FALL THROU */


		/* It's a cloasing tag or opening after '/' char; waiting for '>'. */
	case TAG_CLOSING:
		p = buffer.find_first_not_of(" \t\n\f\v", pos);
		if (p == std::string::npos) {
			buffer.clear();
			pos = 0;
			break;
		}

		if (data[p] != '>') {
			throw Error("Expecting '>'");
		}

		token.type = ELEMENT_CLOSE;
		token.data = stack.back();
		stack.pop_back();
		state = stack.empty() ? START : CDATA;
		dataStart = pos = p + 1;
		break;


		/* It's opening tag and we have read element name */
	case TAG_INSIDE:
		p = buffer.find_first_not_of(" \t\n\f\v", pos);

		pos = p + 1;
		if (p == std::string::npos) {
			buffer.clear();
			pos = 0;
		} else if (data[p] == '/') {
			token.type = TAG_CLOSE;
			state = TAG_CLOSING;
		} else if (data[p] == '>') {
			token.type = TAG_CLOSE;
			dataStart = pos;
			state = CDATA;
		} else if (isNameChar(data[p])) {
			state = ATTR;
			dataStart = pos - 1;
			goto state_attr;
		} else {
			throw Error("Expecting '/', '>' or attribute name.");
		}
		break;


		/* We are reading attribute name */
	state_attr:
	case ATTR:
		const char *it = data + pos, *const end = data + buffer.length();
		while (it != end && isNameChar(*it)) ++it;
		if (it==end) {
			goto shorten_buffer;
		}

		pos = it - data;
		if (pos == dataStart) {
			throw Error("Expecting attribute name.");
		}

		state = ATTR_GOT_NAME;
		token.type = ATTR_NAME;
		token.data.assign(data + dataStart, pos - dataStart);
		break;


		/* Got attribute name, waiting for '=' */
	case ATTR_GOT_NAME:
		p = buffer.find_first_not_of(" \t\n\f\v", pos);
		if (p == std::string::npos) {
			buffer.clear();
			pos = 0;
			break;
		}

		if (data[p] != '=') {
			throw Error("Expecting '='.");
		}

		pos = p + 1;
		state = ATTR_GOT_EQ;
		/* FALL THROU */


		/* Got attribute name and '=', waiting for '"' */
	case ATTR_GOT_EQ:
		p = buffer.find_first_not_of(" \t\n\f\v", pos);
		if (p == std::string::npos) {
			buffer.clear();
			pos = 0;
			break;
		}

		if (data[p] != '"') {
			throw Error("Expecting '\"'.");
		}

		dataStart = pos = p + 1;
		state = ATTR_RD_VALUE;
		/* FALL THROU */


		/* Got attribute name, reading value */
	case ATTR_RD_VALUE:
		p = buffer.find_first_of("\"<>", pos);
		if (p == std::string::npos) {
			goto shorten_buffer;
		}

		if (data[p] != '"') {
			throw Error("Expecting '\"'");
		}

		token.type = ATTR_VALUE;
		token.data.assign(data + dataStart, p - dataStart);
		state = TAG_INSIDE;
		pos = p + 1;
		break;

	default:
		assert(0);
	}


	if (0) {
 shorten_buffer:
		if (dataStart) {
			buffer.erase(0, dataStart);
			dataStart = 0;
		}
		pos = buffer.length();
	}
	return token;
}



}

}
