/** \file
 * Methods generating ppcp packets.
 * $Id: ppcp-packets.hpp,v 1.1 2007/12/24 12:30:22 mina86 Exp $
 */

#ifndef H_PPCP_PACKETS_HPP
#define H_PPCP_PACKETS_HPP

#include "xml-parser.hpp"

namespace ppc {
namespace ppcp {


inline std::string ppcpOpen(const std::string &nick) {
	return "<ppcp n=\"" + xml::escape(nick) + "\">";
}

inline std::string ppcpOpen(const std::string &nick, const std::string &to,
                            bool neg = false) {
	return "<ppcp n=\"" + xml::escape(nick) + "\" to:n=\"" +
		xml::escape(to) + (neg ? "\" to:neg=\"neg\">" : "\">");
}

inline std::string ppcpClose() {
	return "</ppcp>";
}


}
}

#endif
