/** \file
 * User structures definitions.
 * $Id: user.hpp,v 1.1 2007/12/08 18:01:30 mina86 Exp $
 */

#ifndef H_USER_HPP
#define H_USER_HPP

#include <string>

#include "exception.hpp"


namespace ppc {

/** Exception thrown when invalid nick name was once given. */
struct InvalidNick : public Exception {
	/**
	 * Constructor.
	 * \param msg error message.
	 */
	InvalidNick(const std::string &msg) : Exception(msg) { }
};



/**
 * Structure representing single user or more preciselly common subset
 * of properties that user has in all networks.
 */
struct User {
	/** Possible user states. */
	enum State {
		/** User has disconnected from netwock.  Used by networks in
		    \c /net/status/changed signal This is not the same as
		    OFFLINE. */
		DISCONNECTED = -1,
		/** User is OFFLINE but we know he is connected.  This
		    happens if user sent us some data.  */
		OFFLINE,
		ONLINE,   /**< User is online. */
		AWAY,     /**< User is away. */
		XAWAY,    /**< User is extended away. */
		BUSY      /**< User is busy. */
	};


	/** User's status -- state and status message. */
	struct Status {
		/**
		 * Default constructor.  Sets user's state and status message.
		 *
		 * \param st  user's state.
		 * \param msg user's status message.
		 */
		Status(enum State st = OFFLINE,
		       const std::string &msg = std::string())
			: state(st), message(msg) { };

		/**
		 * Copy constructor.
		 * \param st object to copy.
		 */
		Status(const Status &st) : state(st.state), message(st.message) { }

		/** Returns user's state. */
		operator enum State() const { return state; }

		/** User's state. */
		enum State state;
		/** User's status message. */
		std::string message;
	};


	/** Pair which identifies user in single network.*/
	struct ID {
		/**
		 * Initialises structure.  \a name is user's name which will
		 * be converted into a nick name using nickFromName() method.
		 * If \a name is empty InvalidNick will be thrown.
		 *
		 * \param name user's name.
		 * \param addr user's IP address.
		 * \throw InvalidNick if name is empty.
		 */
		ID(const std::string &name, unsigned long addr = 0)
			: nick(requireValidNick(nickFromName(name))), address(addr) { }

		/** User's nick name. */
		std::string nick;
		/** Users IP address. */
		unsigned long address;
	};


	/**
	 * Checks if given string is valid nick name.  Valid nick name
	 * consists of US-ASCII characters with codes from 48 to 127
	 * expect for upper-case characters.
	 * \param nick nick name.
	 * \return whether nick name is valid.
	 */
	static bool isValidNick(const std::string &nick);

	/**
	 * Checks if given string is valid display name.  Valid display
	 * name may contain any UTF-8 characters expect for characters
	 * with codes less then 48.
	 * \param name display name.
	 * \return whether display name is valid.
	 */
	static bool isValidName(const std::string &name);

	/**
	 * Converts display name into nick name.  Conversion is done in
	 * the following name: upper case letters are replaced with lower
	 * case letters and other characters which are not valid in nick
	 * name are replaced with underscore.  Name does not need to be
	 * valid name -- all characters which are not valid display name
	 * characters are converted into underscore as well.
	 *
	 * \param name display name.
	 * \return nick name created from \a name.
	 */
	static std::string nickFromName(const std::string &name) {
		std::string result = name;
		nickFromNameInPlace(result);
		return result;
	}

	/**
	 * Converts display name into nick name in place.  Conversion is
	 * done in the following name: upper case letters are replaced
	 * with lower case letters and other characters which are not
	 * valid in nick name are replaced with underscore.  Name does not
	 * need to be valid name -- all characters which are not valid
	 * display name characters are converted into underscore as well.
	 *
	 * \param name display name.
	 * \return reference t argument after converting it into nick
	 *         name.
	 */
	static std::string &nickFromNameInPlace(std::string &name);



	/** User's identificator. */
	ID id;
	/** User's display name. */
	std::string name;
	/** User's status. */
	Status status;



protected:
	/**
	 * Throws exception if nick name is not valid.
	 * \param nick nick name to validte.
	 * \return \a nick.
	 * \throw InvalidNick if nick name is invalid.
	 */
	static const std::string &requireValidNick(const std::string &nick) {
		if (!isValidNick(nick)) {
			throw InvalidNick("Invalid nick name: '" + nick + '\'');
		}
		return nick;
	}
};


}

#endif
