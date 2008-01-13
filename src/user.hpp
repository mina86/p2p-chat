/** \file
 * User structures definitions.
 * $Id: user.hpp,v 1.9 2008/01/13 21:55:54 mina86 Exp $
 */

#ifndef H_USER_HPP
#define H_USER_HPP

#include <string>

#include "exception.hpp"
#include "netio.hpp"


namespace ppc {


class Network;



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


	/** Pair which identifies user in single network. */
	struct ID {
		/**
		 * Initialises structure.  \a name is user's name which will
		 * be converted into a nick name using nickFromName() method.
		 * If \a name is empty InvalidNick will be thrown.
		 *
		 * \param name user's name.
		 * \param addr user's address (IP, port pair).
		 * \throw InvalidNick if name is not empty and invalid display name.
		 */
		ID(const std::string &name, Address addr)
			: nick(nickFromName(name)), address(addr) {
			if (!name.empty() && !isValidName(name)) {
				throw InvalidNick("Invalid display name: '" + name + '\'');
			}
		}

		/** Returns identifier as a string. */
		std::string toString() const {
			std::string result = nick;
			result += '/';
			return result += address.toString();
		}

		/**
		 * Returns identifier as a string.
		 * \param name user's display name
		 */
		std::string toString(const std::string &name) const {
			std::string result = name;
			result += '(';
			if (!User::nameMatchesNick(nick, name)) {
				result += nick;
				result += '/';
			}
			result += address.toString();
			return result += ')';
		}

		/** User's nick name. */
		std::string nick;
		/** Users IP address and port number. */
		Address address;
	};


	/**
	 * Checks if given string is valid nick name.  Valid nick name
	 * consists of US-ASCII characters with codes from 48 to 127
	 * except for upper-case characters.
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
		return nickFromNameInPlace(result);
	}

	/**
	 * Returns \c true iff display name converted into nick name
	 * matches given nick name.
	 *
	 * \param name display name.
	 * \param nick nick name.
	 */
	static bool nameMatchesNick(const std::string &name,
	                            const std::string &nick);

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

	/**
	 * Returns state name.
	 * \param state state to get name of.
	 */
	static const char *stateName(State state);

	/**
	 * Returns state from it's name.
	 * \param state state's name.
	 */
	static State getState(const std::string &state) {
		bool ignore;
		return getState(state, ignore);
	}

	/**
	 * Returns state from it's name.
	 * \param state state's name.
	 * \param valid set whether \a state was valid state's name.
	 */
	static State getState(const std::string &state, bool &valid);



	/** User's identificator. */
	ID id;
	/** User's display name. */
	std::string name;
	/** User's status. */
	Status status;


	/**
	 * Initialises User object.
	 *
	 * \param i  user's ID -- nick name, IP address pair
	 * \param n  user's display name or empty string.
	 * \param st user's status.
	 * \throw InvalidNick if \a n is invalid display name.
	 * \throw InvalidNick if nick name in \a i is empty.
	 */
	User(ID i, const std::string &n, const Status &st = Status())
		: id(i), name(n.empty() ? i.nick : n), status(st) {
		if (!n.empty() && !isValidName(n)) {
			throw InvalidNick("Invalid display name: '" + n + '\'');
		}
		if (id.nick.empty()) {
			throw InvalidNick("Invalid nick name: ''");
		}
	}


	/**
	 * Initialises User object.  User's display name is set from
	 * <tt>i.name</tt>.
	 *
	 * \param i  user's ID -- nick name, IP address pair
	 * \param st user's status.
	 * \throw InvalidNick if nick name in \a i is empty.
	 */
	explicit User(ID i, const Status &st = Status())
		: id(i), name(i.nick), status(st) { }


	/**
	 * Initialises User object.  User's ID is set from \a n and \a
	 * addr.
	 *
	 * \param n    user's display name.
	 * \param addr user's address (IP, port pair).
	 * \param st   user's status.
	 * \throw InvalidNick if \a n is invalid display name.
	 * \throw InvalidNick if nick name in \a i is empty.
	 */
	User(const std::string &n, Address addr, const Status &st = Status())
		: id(n, addr), name(n), status(st) {
		if (!isValidName(n)) {
			throw InvalidNick("Invalid display name: '" + n + '\'');
		}
		if (id.nick.empty()) {
			throw InvalidNick("Invalid nick name: ''");
		}
	}


	/**
	 * Returns user name with all informations.  If user's display
	 * name matches user's nick name (see \link
	 * User::nickFromName(const std::string &)\endlink ) returned
	 * string is display name followed by IP address and port number
	 * in parenthesis.  Otherwise, IP address is preceded by user's
	 * nick name and a slash sign.
	 */
	std::string formattedName() const {
		return id.toString(name);
	}
};



/**
 * Returns \c true if User::ID objects are equal.  Objects are equal
 * if they have the same address and nick name.
 * \param a first User::ID object to compare.
 * \param b second User::ID object to compare.
 */
inline bool operator==(const User::ID &a, const User::ID &b) {
	return a.address == b.address && a.nick == b.nick;
}


/**
 * Returns \c true if User::ID objects are not equal.  Objects are
 * equal if they have the same address and nick name.
 * \param a first User::ID object to compare.
 * \param b second User::ID object to compare.
 */
inline bool operator!=(const User::ID &a, const User::ID &b) {
	return !(a == b);
}


/**
 * User::ID linear order.  This order is defined as follows: <tt>a <=
 * b iff (a.address, a.nick.length, a.nick) <= (b.address,
 * b.nick.length, b.nick)</tt> which means that ID's are first ordered
 * by address, then by nick's length and then (only if nick's lengths
 * are equal) lexicographically on nicks.
 *
 * \param a first User::ID object to compare.
 * \param b second User::ID object to compare.
 * \return \c true if first object is greater then the second.
 */
inline bool operator> (const User::ID &a, const User::ID &b) {
	return a.address > b.address ||
		(a.address == b.address &&
		 (a.nick.length() > b.nick.length() ||
		  (a.nick.length() == b.nick.length() && a.nick > b.nick)));
}


/**
 * User::ID linear order.  This order is defined as follows: <tt>a <=
 * b iff (a.address, a.nick.length, a.nick) <= (b.address,
 * b.nick.length, b.nick)</tt> which means that ID's are first ordered
 * by address, then by nick's length and then (only if nick's lengths
 * are equal) lexicographically on nicks.
 *
 * \param a first User::ID object to compare.
 * \param b second User::ID object to compare.
 * \return \c true if first object is greater then or equal to the second.
 */
inline bool operator>=(const User::ID &a, const User::ID &b) {
	return a.address > b.address ||
		(a.address == b.address &&
		 (a.nick.length() > b.nick.length() ||
		  (a.nick.length() == b.nick.length() && a.nick >= b.nick)));
}


/**
 * User::ID linear order.  This order is defined as follows: <tt>a <=
 * b iff (a.address, a.nick.length, a.nick) <= (b.address,
 * b.nick.length, b.nick)</tt> which means that ID's are first ordered
 * by address, then by nick's length and then (only if nick's lengths
 * are equal) lexicographically on nicks.
 *
 * \param a first User::ID object to compare.
 * \param b second User::ID object to compare.
 * \return \c true if first object is less then the second.
 */
inline bool operator< (const User::ID &a, const User::ID &b) {
	return b > a;
}


/**
 * User::ID linear order.  This order is defined as follows: <tt>a <=
 * b iff (a.address, a.nick.length, a.nick) <= (b.address,
 * b.nick.length, b.nick)</tt> which means that ID's are first ordered
 * by address, then by nick's length and then (only if nick's lengths
 * are equal) lexicographically on nicks.
 *
 * \param a first User::ID object to compare.
 * \param b second User::ID object to compare.
 * \return \c true if first object is less then or equal to the second.
 */
inline bool operator<=(const User::ID &a, const User::ID &b) {
	return b >= a;
}


}

#endif
