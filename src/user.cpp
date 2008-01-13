/** \file
 * User structures definitions.
 * $Id: user.cpp,v 1.2 2008/01/13 11:57:12 mina86 Exp $
 */

#include "user.hpp"

namespace ppc {


bool User::isValidNick(const std::string &nick) {
	const char *it = nick.data(), *const end = it + nick.length();
	bool ok = it != end;
	for (; ok && it!=end; ++it) {
		const char ch = *it;
		ok = ok && static_cast<signed char>(ch)>=48 && (ch<'A' || ch>'Z');
	}
	return ok;
}


bool User::isValidName(const std::string &name) {
	const char *it = name.data(), *const end = it + name.length();
	bool ok = it != end;
	for (; ok && it!=end; ++it) {
		ok = ok && static_cast<signed char>(*it)>=48;
	}
	return ok;
}


std::string &User::nickFromNameInPlace(std::string &name) {
	char *it = &name[0], *const end = it + name.length();
	for (; it != end; ++it) {
		if (static_cast<signed char>(*it) < 48) {
			*it = '_';
		} else if (*it >= 'A' && *it <= 'Z') {
			*it |= 32;
		}
	}
	return name;
}


bool User::nameMatchesNick(const std::string &name, const std::string &nick) {
	if (name.length() != nick.length()) {
		return false;
	}
	if (name.empty()) {
		return true;
	}

	const char *it1 = name.data(), *it2 = nick.data();
	const char *const end = it1 + name.length();
	char cmp;

	do {
		if (static_cast<signed char>(*it1) < 48) {
			cmp = '_';
		} else if (*it1 >= 'A' && *it1 <= 'Z') {
			cmp = *it1 | 32;
		} else {
			cmp = *it1;
		}
	} while (*it2++ == cmp && ++it1 != end);

	return it1 == end;
}


const char *User::stateName(enum State state) {
	switch (state) {
	case OFFLINE: return "offline";
	case ONLINE : return "online";
	case AWAY   : return "away";
	case XAWAY  : return "extended away";
	case BUSY   : return "busy";
	default     :
		assert(0);
		return "<unknown>";
	}
}


}
