/** \file
 * XML parser definition.
 * $Id: xml-parser.hpp,v 1.4 2007/12/10 11:55:42 mina86 Exp $
 */

#ifndef H_XML_PARSER_HPP
#define H_XML_PARSER_HPP

#include <string>
#include <vector>
#include <map>

#include "exception.hpp"


namespace ppc {


/**
 * Namespace with functions and structures used to parse XML data.
 */
namespace xml {



/**
 * Checks if given character is valid character for element and
 * attribute name.  This includes digits, letters, colon,
 * underscore and dash.  It's probably not what XML standard
 * defines... oh well...
 *
 * \param ch character to test.
 * \return whether character is valid name.
 */
inline bool isNameChar(char ch) {
	return (ch>='0' && ch<='9') || ((ch | 64)>='a' && (ch | 64)<='z') ||
		ch == ':' || ch=='-' || ch=='_';
}

/**
 * Checks if character is white space character.  This includes
 * space, tabulator, new line, carriage return, vertical tabular,
 * form feed.  As with isnamechar() XML probably things a white
 * space character is something else but we don't care.
 *
 * \param ch character to test.
 * \return whether character is valid name.
 */
inline bool isSpace(char ch) {
	return ch == ' ' || ch=='\t' || ch=='\n' || ch=='\r' || ch=='\v'
		|| ch=='\f';
}


/**
 * Replaces entities in string in place.  If you forgot, entiti is
 * en "escape sequence" which starts with ampersand and ends with
 * semicolon (ie. \&gt;).  Only "gt", "lt", "amp" and numeric
 * (\&#\<number\>;, \&\#x\<hexnumber\>;).
 *
 * \param str string to parse.
 * \return reference to str (after parsing entities).
 * \throw Error if \a str contains invalid or missformatted entities.
 */
std::string &unescapeInPlace(std::string &str);


/**
 * Replaces entities in string.  If you forgot, entiti is en
 * "escape sequence" which starts with ampersand and ends with
 * semicolon (ie. \&gt;).  Only "gt", "lt", "amp" and numeric
 * (\&#\<number\>;, \&\#x\<hexnumber\>;).
 *
 * \param str string to parse.
 * \return string with entities replaced with characters.
 * \throw Error if \a str contains invalid or missformatted entities.
 */
inline std::string unescape(const std::string &str) {
	std::string data(str);
	return unescapeInPlace(data);
}


/**
 * Replaces special characters into entities.  This does the
 * opposite of parseEntities method.  Characters which are
 * replaced are greater then sign (\<), lower then sign (\>),
 * ampersand (\&), quote sign ("), apostrophe (') and a NUL byte.
 * They are all replaced into numeric entities.
 *
 * \param str string to parse.
 * \return string with special characters replaced with entities.
 */
std::string escape(const std::string &str);



/**
 * Exception thrown by XML tokenizer and parser.
 */
struct Error : public Exception {
	/**
	 * Constructor.
	 * \param msg error message.
	 */
	Error(const std::string &msg) : Exception(msg) { }
};


/**
 * XML tokenizer divides XML data into "tokens" such as tag open,
 * attribute, tag close, etc.  It does not require vhole data to be
 * given at once, instead one may feed it vith chunk at a time.
 */
struct Tokenizer {
	/** Token type. */
	enum Type {
		END        =  0,  /**< No more tokens. */
		TAG_OPEN,         /**< Tag opening an element is opened. */
		ATTR_NAME,        /**< An attribute name. */
		ATTR_VALUE,       /**< An attribute value. */
		TAG_CLOSE,        /**< Tag opening an element is closed. */
		TEXT,             /**< CData. */
		ELEMENT_CLOSE     /**< An element is closed. */
	};


	/** Token. */
	struct Token {
		/**
		 * Constructor.  It's caller's responsibility to guarantee
		 * that all arguments are valid.
		 *
		 * \param t token's type.
		 * \param d token's data.
		 */
		Token(enum Type t = END, const std::string &d = std::string())
			: type(t), data(d) { }

		/** Returns token type. */
		operator enum Type() const { return type; }

		/** Returns \c true if token's type is not \c END. */
		operator bool() const { return type != END; }

		/** Token's type. */
		enum Type type;

		/**
		 * Token's data.  If \a type is \c TAG_OPEN or \c
		 * ELEMENT_CLOSE it is the name of element token refers to.
		 * If \a type is \c ATTR_NAME it is attribute's tame.  If \a
		 * type is \c ATTR_VALUE it is attribute's value.  If \a type
		 * is \c TEXT it is cdata.  Otherwise its value is meaning
		 * less.
		 */
		std::string data;
	};


	/** Initialises tokenizer and zeroes its state. */
	void init() {
		stack.clear();
		buffer.clear();
		state = 0;
		pos = 0;
	}

	/**
	 * Feeds tokenizer with data.
	 * \param data data to feed tokenizer with.
	 */
	void feed(const std::string &data) {
		buffer += data;
	}

	/**
	 * Feeds tokenizer with data.
	 * \param data data to feed tokenizer with.
	 */
	void feed(const char *data) {
		buffer += data;
	}

	/**
	 * Feeds tokenizer with data.
	 * \param data data to feed tokenizer with.
	 * \param len  data's length.
	 */
	void feed(const char *data, std::string::size_type len) {
		buffer.append(data, len);;
	}

	/**
	 * Returns next token, \c END if there are no more tokens.
	 * \throw Error if data is missformatted.
	 */
	Token nextToken();

	/**
	 * If tokenizer expects more data exreption is thrown otherwise no
	 * action is taken.
	 * \throw Error if tokenizer is in state that does not allow end of data.
	 */
	void done() {
		if (!stack.empty() || state || !buffer.empty()) {
			throw Error("Unexpected end of data.");
		}
	}


	/**
	 * Return's current parser's elements stack.  The first element of
	 * stack is name of the root element and last element is the
	 * deepest element.
	 * \return opened elements stack.
	 */
	const std::vector<std::string> &getStack() const { return stack; }


private:
	/** Stack of opened elements. */
	std::vector<std::string> stack;

	std::string buffer;               /**< Internal buffer. */
	std::string::size_type pos;       /**< Internal variable. */
	std::string::size_type dataStart; /**< Internal variable. */


	/** Parser's state. */
	unsigned state;
};



/**
 * Abstract XML parser.  XML parses should inherit from this class and
 * overwrite open(), attribute(), close() and cdata() methods to build
 * real parser.  If instead of feeding class with tokens one prefers
 * giving it a string and let it divide it into tokens by itself one
 * may use Parser class.  If instead of having open() and attribute()
 * methods one prefers to have single open() method called with list
 * of arguments one may use Parser2 class.
 */
struct TokenConsumer {
	/**
	 * Consumes tokens produced by \a tokenizer untill \c END token.
	 * \param tokenizer tokenizer producing tokens.
	 * \throw Error if data is missformatted.
	 */
	void consumeTokens(Tokenizer &tokenizer) {
		while (consumeToken(tokenizer.nextToken()));
	}


	/**
	 * Consumes single token.
	 * \param token token to consume.
	 * \throw Error if data is missformatted.
	 * \return false iff token was \c END token.
	 */
	bool consumeToken(const Tokenizer::Token &token) {
		switch (token.type) {
		case Tokenizer::END: return false;
		case Tokenizer::TAG_OPEN:       open(token.data); break;
		case Tokenizer::TEXT:          cdata(token.data); break;
		case Tokenizer::ELEMENT_CLOSE: close(token.data); break;
		case Tokenizer::ATTR_NAME: attrName= token.data ; break;
		case Tokenizer::ATTR_VALUE:
			attribute(attrName, token.data);
			attrName.clear();
		case Tokenizer::TAG_CLOSE:
			break;
		}

		return true;
	}


	/** Destructor. */
	virtual ~TokenConsumer() { }


protected:
	/**
	 * Called when new element is opened.
	 * \param name element's name.
	 */
	virtual void open(const std::string &name) = 0;

	/**
	 * Called when new attribute is added to element.
	 * \param name attribute name.
	 * \param value attribute's value.
	 */
	virtual void attribute(const std::string &name,
	                       const std::string &value) = 0;

	/**
	 * Called when open tag is closed.  This is called after open()
	 * and optional attribute() calls.
	 */
	virtual void open_end() { }

	/**
	 * Called when element is being closed.
	 * \param name element's name.
	 */
	virtual void close(const std::string &name) = 0;

	/** Called when there is some CData ready.
	 * \param data CData. */
	virtual void cdata(const std::string &data) = 0;


private:
	/** Attribute name. */
	std::string attrName;
};




/**
 * Abstract XML parser.  XML parses should inherit from this class and
 * overwrite open(), attribute(), close() and cdata() methods to build
 * real parser.  If instead of having open() and attribute() methods
 * one prefers to have single open() method called with list of
 * arguments one may use Parser2 class.
 */
struct Parser : protected TokenConsumer {
	/** Initialises parser and zeroes its state. */
	void init() {
		tokenizer.init();
	}

	/**
	 * Feeds parser with data.
	 * \param data data to feed parser with.
	 * \throw Error if data is missformatted.
	 */
	void feed(const std::string &data) {
		tokenizer.feed(data);
		consumeTokens(tokenizer);
	}

	/**
	 * Feeds parser with data.
	 * \param data data to feed parser with.
	 * \throw Error if data is missformatted.
	 */
	void feed(const char *data) {
		tokenizer.feed(data);
		consumeTokens(tokenizer);
	}

	/**
	 * Feeds parser with data.
	 * \param data data to feed parser with.
	 * \param len  data's length.
	 * \throw Error if data is missformatted.
	 */
	void feed(const char *data, std::string::size_type len) {
		tokenizer.feed(data, len);
		consumeTokens(tokenizer);
	}

	/**
	 * If parser expects more data exreption is thrown otherwise no
	 * action is taken.
	 * \throw Error if parser is in state that does not allow end of data.
	 */
	void done() {
		tokenizer.done();
	}


private:
	/** Tokenizer used to parse data. */
	Tokenizer tokenizer;
};



/**
 * Abstract XML parser.  It extends Parser class by replacing
 * open(std::string), attribute() and open_end() with single
 * open(std::string,Attributes) method which in some situations may be
 * easier to use.
 */
struct Parser2 : public Parser {
	/** Type for holding list of attribtues. */
	typedef std::map<std::string, std::string> Attributes;


protected:
	/**
	 * Called when open tag is encountered.
	 *
	 * \param name  element name.
	 * \param attrs list of attributes.
	 */
	virtual void open(const std::string &name, const Attributes &attrs) = 0;

	/**
	 * Called when open tag is encountered.  Instead of this method
	 * \link open(const std::string&, const Attributes&) \endlink
	 * method should be used.  If you intend to use it consider
	 * Parser class instead of Parser2.
	 * \param name element name.
	 */
	virtual void open(const std::string &name) {
		element = name;
		attributes.clear();
	}

	/**
	 * Called when element attribute is encountered.  Instead of this
	 * method \link open(const std::string&, const Attributes&)
	 * \endlink method should be used.  If you intend to use it
	 * consider Parser class instead of Parser2.
	 * \param name attribute's name.
	 * \param value attribute's value.
	 */
	virtual void attribute(const std::string &name,
	                       const std::string &value) {
		attributes[name] = value;
	}

	/**
	 * Called when open tag is closed.  Instead of this method \link
	 * open(const std::string&, const Attributes&) \endlink method
	 * should be used.  If you intend to use it consider Parser
	 * class instead of Parser2.
	 */
	virtual void open_end() {
		open(element, attributes);
		element.clear();
		attributes.clear();
	};


private:
	/** Current element. */
	std::string element;
	/** List of element attributes. */
	Attributes attributes;
};


}

}

#endif
