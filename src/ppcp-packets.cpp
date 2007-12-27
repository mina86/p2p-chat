/** \file
 * Methods generating ppcp packets.
 * $Id: ppcp-packets.cpp,v 1.2 2007/12/27 17:58:36 mina86 Exp $
 */

#include <stdio.h>

#include "ppcp-packets.hpp"

namespace ppc {
namespace ppcp {

/** Static buffer for internal user.  Code is not thread safe anyway. */
static char buffer[1024];


std::string ppcpOpen(const User &user) {
	sprintf(buffer, "\" p=\"%u\">", user.id.address.port);
	return "<ppcp n=\"" +
		xml::escape(User::nickFromName(user.name) == user.id.nick
		            ? user.name : user.id.nick) + buffer;
}


std::string ppcpOpen(const User &user, const std::string &to, bool neg) {
	sprintf(buffer, !to.empty() ? neg ? "\" p=\"%u\" to:neg=\"neg\" to:n=\""
	                                  : "\" p=\"%u\" to:n=\""
	                            : "\" p=\"%u\">",
	         user.id.address.port);

	std::string packet = "<ppcp n=\"" +
		xml::escape(User::nickFromName(user.name) == user.id.nick
		            ? user.name : user.id.nick) + buffer;
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
	default           : packet = "<st"            ; break;
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
