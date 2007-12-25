/** \file
 * Methods generating ppcp packets.
 * $Id: ppcp-packets.hpp,v 1.2 2007/12/25 01:32:41 mina86 Exp $
 */

#ifndef H_PPCP_PACKETS_HPP
#define H_PPCP_PACKETS_HPP

#include "xml-parser.hpp"

namespace ppc {
namespace ppcp {


/**
 * Returns a \c ppcp open tag.
 * \param nick value of \c n atribute.
 */
inline std::string ppcpOpen(const std::string &nick) {
	return "<ppcp n=\"" + xml::escape(nick) + "\">";
}

/**
 * Returns a \c ppcp open tag.
 * \param nick value of \c n attribute.
 * \param to   value of \c to:n attribute.
 * \param neg  if \c true \c to:neg will be set to \c "neg".
 */
inline std::string ppcpOpen(const std::string &nick, const std::string &to,
                            bool neg = false) {
	return "<ppcp n=\"" + xml::escape(nick) + "\" to:n=\"" +
		xml::escape(to) + (neg ? "\" to:neg=\"neg\">" : "\">");
}

/** Returns a \c ppcp close tag (that is <tt>"\</ppcp\>"</tt>). */
inline std::string ppcpClose() {
	return "</ppcp>";
}


}
}

#endif
