/** \file
 * PPCP parser definition.
 * $Id: ppcp-parser.hpp,v 1.9 2008/01/04 12:39:56 mina86 Exp $
 */

#ifndef H_PPCP_PARSER_HPP
#define H_PPCP_PARSER_HPP

#include <string>

#include "xml-parser.hpp"
#include "user.hpp"


namespace ppc {


/**
 * Namespace containing stuff connected with parsing and creating PPCP
 * packets.
 */
namespace ppcp {


/**
 * PPCP packets tokenizer that consumes tokens generated by
 * xml::Tokenizer.  If you don't like the idea of maintaining separate
 * xml::Tokenizer you may use StandAloneTokenizer instead.
 */
struct Tokenizer {
	/** Token's type. */
	enum Type {
		END = 0,       /**< There are no more tokens so far. */
		IGNORE,        /**< Packet shall be ignored. */
		PPCP_OPEN,     /**< \c ppcp element opened. */
		ST,            /**< Got \c st element. */
		RQ,            /**< Got \c rq element. */
		M,             /**< Got \c m element. */
		PPCP_CLOSE     /**< \c ppcp element closed. */
	};


	/** Single PPCP token. */
	struct Token {
		/** Token's type. */
		enum Type type;

		/**
		 * Meaning of data depends on type.  For PPCP_OPEN it is
		 * user's nick name (or more preciselly value of \c
		 * n attribute.  For ST it is status message.  For M it is
		 * message's body.
		 */
		std::string data;

		/**
		 * Meaning of secondary data depends on type.  For PPCP_OPEN
		 * it is the oryginal value of \a n attribute.  For PPCP_ST
		 * it's a display name or empty string if \a dn attribute was
		 * missing.
		 */
		std::string data2;

		/** Possible flags when type is M. */
		enum {
			M_ACTION  = 1,
			M_MESSAGE = 2
		};

		/**
		 * Meaning of flags depends on type.  For PPCP_OPEN it's value
		 * of \a p attribute.  For ST this is user's state cast to
		 * unsigned (see User::State).  For M this is combination of
		 * M_ACTION and M_MESSAGE.
		 */
		unsigned flags;


		/**
		 * Constructs Token from given parameters.
		 * \param t token's type.
		 * \param d token's data.
		 * \param f token's flags.
		 */
		Token(enum Type t = END, const std::string &d = std::string(),
		      unsigned f = 0) : type(t), data(d), flags(f) { }


		/**
		 * Construct Token from given paremeters with empty data.
		 * Default constructor set's type to END and zeroes flags.
		 *
		 * \param t token's type.
		 * \param f token's flags.
		 */
		Token(enum Type t, unsigned f)
			: type(t), data(), flags(f) { }


		/** Returns \c true iff token's type is not END. */
		operator bool() const { return type != END; }

		/** Returns token's type. */
		operator enum Type() const { return type; }
	};


	/**
	 * Constructs tokenizer.  \a nick is our nick name which is
	 * compared witha \c to:n attribute of \c ppcp element.  Moreover,
	 * if \a port is non-zero \a nick is compared with \c n attribute
	 * and \c port with \c p attribute and if they both maches packet
	 * is ignored.
	 *
	 * \param nick our nick name.
	 * \param port port number to compare against \c p attribute.
	 */
	explicit Tokenizer(const std::string &nick, Port port = 0)
		: ourNick(nick), ourPort(port) { }


	/**
	 * Constructs tokenizer.  This constructor does the same \link
	 * Tokenizer(const std::string&, Port) \endlink does but takes
	 * both arguments from an User::ID object.
	 *
	 * \param id User::ID object including with nick name and port number.
	 */
	explicit Tokenizer(const User::ID &id)
		: ourNick(id.nick), ourPort(id.address.port) { }



	/**
	 * Zeroe's tokenizer state.
	 */
	void init();


	/**
	 * Zeroe's tokenizer state and sets our port number.
	 * \param port port number to compare against \c p attribute.
	 */
	void init(Port port) {
		ourPort = port;
		init();
	}


	/**
	 * Takes given XML token and returns next PPCP token.  It may be
	 * a \c END token.
	 * \param token token to consume.
	 */
	Token nextToken(const xml::Tokenizer::Token &token);


	/**
	 * Consumes tokens from \a tokenizer until either tokenizer gives
	 * END token or there is new PPCP token.
	 * \param tokenizer XML tokenizer to consume tokens from.
	 */
	Token nextToken(xml::Tokenizer &tokenizer);



private:
	/** Our user's nick name. */
	std::string ourNick;
	/** Our port number to compare agains \c p attribute. */
	unsigned short ourPort;

	/** Current element. */
	unsigned char element;
	/** Current attribute. */
	unsigned char attribute;
	/** Current state flags. */
	unsigned short flags;
	/** Whether element should be ignored and how many levels deep. */
	unsigned ignore;

	/** Holds content of \c st or \c m elements or \c n attribute of
	    \c ppcp element. */
	std::string data;
	/** Holds content of \c dn attribute of \c ppcp element. */
	std::string data2;
};



/**
 * A stand alone PPCP tokenizer which does not need a separate XML
 * tokenizer object since it has it built in.
 */
struct StandAloneTokenizer {
	/**
	 * Constructs tokenizer.  \a nick is our nick name which is
	 * compared witha \c to:n attribute of \c ppcp element.  Moreover,
	 * if \a port is non-zero \a nick is compared with \c n attribute
	 * and \c port with \c p attribute and if they both maches packet
	 * is ignored.
	 *
	 * \param nick our nick name.
	 * \param port port number to compare against \c p attribute.
	 */
	explicit StandAloneTokenizer(const std::string &nick, Port port = 0)
		: ppcpTokenizer(nick, port) { }

	/**
	 * Constructs tokenizer.  This constructor does the same \link
	 * StandAloneTokenizer(const std::string&, Port) \endlink does but
	 * takes both arguments from an User::ID object.
	 *
	 * \param id User::ID object including with nick name and port number.
	 */
	explicit StandAloneTokenizer(const User::ID &id) : ppcpTokenizer(id) { }

	/**
	 * Zeroe's tokenizer state.
	 */
	void init() {
		ppcpTokenizer.init();
		xmlTokenizer.init();
	}

	/**
	 * Zeroe's tokenizer state and sets our port number.
	 * \param port port number to compare against \c p attribute.
	 */
	void init(Port port) {
		ppcpTokenizer.init(port);
		xmlTokenizer.init();
	}

	/**
	 * Feeds tokenizer with data.
	 * \param data data to feed tokenizer with.
	 */
	void feed(const std::string &data) {
		xmlTokenizer.feed(data);
	}

	/**
	 * Feeds tokenizer with data.
	 * \param data data to feed tokenizer with.
	 */
	void feed(const char *data) {
		xmlTokenizer.feed(data);
	}

	/**
	 * Feeds tokenizer with data.
	 * \param data data to feed tokenizer with.
	 * \param len  data's length.
	 */
	void feed(const char *data, std::string::size_type len) {
		xmlTokenizer.feed(data, len);
	}

	/**
	 * Returns next token, \c END if there are no more tokens.
	 * \throw xml::Error if data is missformatted.
	 */
	Tokenizer::Token nextToken() {
		return ppcpTokenizer.nextToken(xmlTokenizer);
	}

	/**
	 * If tokenizer expects more data exreption is thrown otherwise no
	 * action is taken.
	 * \throw xml::Error if tokenizer is in state that does not allow
	 *                   end of data.
	 */
	void done() {
		xmlTokenizer.done();
	}


private:
	/** Underlaying ppcp::Tokenizer. */
	Tokenizer ppcpTokenizer;

	/** Underlaying xml::Tokenizer. */
	xml::Tokenizer xmlTokenizer;
};


}

}

#endif
