/** \file
 * User structures definitions.
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

#include <limits.h>

#include "user.hpp"

namespace ppc {


bool User::isValidNick(const std::string &nick) {
	const char *it = nick.data(), *const end = it + nick.length();
	bool ok = it != end;
	for (; ok && it!=end; ++it) {
		const char ch = *it;
#if CHAR_MAX < 128
		ok = ok && ch>=48 && (ch<'A' || ch>'Z');
#else
		ok = ok && ch>=48 && ch<128 && (ch<'A' || ch>'Z');
#endif
	}
	return ok;
}


bool User::isValidName(const std::string &name) {
	const char *it = name.data(), *const end = it + name.length();
	bool ok = it != end;
	for (; ok && it!=end; ++it) {
#if CHAR_MAX < 128
		ok = ok && *it >= 48;
#else
		ok = ok && *it >= 48 && *it < 128;
#endif
	}
	return ok;
}


std::string &User::nickFromNameInPlace(std::string &name) {
	char *it = &name[0], *const end = it + name.length();
	for (; it != end; ++it) {
#if CHAR_MAX < 128
		if (*it < 48) {
#else
		if (*it < 48 || *it >= 128) {
#endif
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


const char *User::stateName(User::State state) {
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


User::State User::getState(const std::string &state, bool &valid) {
	valid = true;
	switch (state.length()) {
	case 7:
		if (state == "offline") return OFFLINE;
		break;
	case 6:
		if (state == "online" ) return ONLINE;
		break;
	case 5:
		if (state == "xaway"  ) return XAWAY;
		break;
	case 4:
		if (state == "away"   ) return AWAY;
		if (state == "busy"   ) return BUSY;
		break;
	case 3:
		if (state == "dnd"    ) return BUSY;
		if (state == "off"    ) return OFFLINE;
		break;
	case 2:
		if (state == "on"     ) return ONLINE;
		if (state == "aw"     ) return AWAY;
		if (state == "xa"     ) return XAWAY;
		break;
	}
	valid = false;
	return ONLINE;
}



}
