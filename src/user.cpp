/** \file
 * User structures definitions.
 * $Id: user.cpp,v 1.1 2007/12/08 18:01:30 mina86 Exp $
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


}
