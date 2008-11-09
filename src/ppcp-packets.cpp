/** \file
 * Methods generating ppcp packets.
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

#include <assert.h>

#include <stdio.h>

#include "ppcp-packets.hpp"

namespace ppc {
namespace ppcp {


std::string ppcpOpen(const User &user) {
	sprintf(sharedBuffer, "\" p=\"%u\">", user.id.address.port.host());
	return "<ppcp n=\"" +
		xml::escape(User::nameMatchesNick(user.name, user.id.nick)
		            ? user.name : user.id.nick) + sharedBuffer;
}


std::string ppcpOpen(const User &user, const std::string &to, bool neg) {
	sprintf(sharedBuffer,
	        !to.empty() ? neg ? "\" p=\"%u\" to:neg=\"neg\" to:n=\""
	                          : "\" p=\"%u\" to:n=\""
	                   : "\" p=\"%u\">",
	        user.id.address.port.host());

	std::string packet = "<ppcp n=\"" +
		xml::escape(User::nameMatchesNick(user.name, user.id.nick)
		            ? user.name : user.id.nick) + sharedBuffer;
	if (!to.empty()) {
		packet += xml::escape(to) + "\">";
	}
	return packet;
}


std::string st(User::State st, const std::string &msg,
               const std::string &name) {
	std::string packet;
	switch (st) {
	case User::OFFLINE: packet = "<st st=\"off\"" ; break;
	case User::AWAY   : packet = "<st st=\"away\""; break;
	case User::XAWAY  : packet = "<st st=\"xa\""  ; break;
	case User::BUSY   : packet = "<st st=\"dnd\"" ; break;
	default           : assert(st == User::ONLINE);
	                    packet = "<st"            ; break;
	}

	if (!name.empty()) {
		packet += " dn=\"" + xml::escape(name) + '"';
	}

	return packet + (msg.empty() ? "/>" : ('>' + xml::escape(msg) + "</st>"));
}


std::string m(const std::string &msg, unsigned flags) {
	std::string packet("<m");
	if (flags & sig::MessageData::ACTION) {
		packet.append(" ac=\"ac\"");
	}
	if (flags & sig::MessageData::MESSAGE) {
		packet.append(" msg=\"msg\"");
	}
	return packet + '>' + xml::escape(msg) + "</m>";
}


}
}
