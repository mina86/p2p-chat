/** \file
 * Methods generating ppcp packets.
 * $Id: ppcp-packets.cpp,v 1.1 2007/12/25 15:35:47 mina86 Exp $
 */

#include "ppcp-packets.hpp"

namespace ppc {
namespace ppcp {


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
