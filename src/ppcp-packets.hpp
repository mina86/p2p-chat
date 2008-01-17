/** \file
 * Methods generating ppcp packets.
 * $Id: ppcp-packets.hpp,v 1.7 2008/01/17 17:32:25 mina86 Exp $
 */

#ifndef H_PPCP_PACKETS_HPP
#define H_PPCP_PACKETS_HPP

#include <string>

#include "xml-parser.hpp"
#include "user.hpp"
#include "signal.hpp"


namespace ppc {
namespace ppcp {


/**
 * Returns a \c ppcp open tag.
 * \param user user object.
 */
std::string ppcpOpen(const User &user);


/**
 * Returns a \c ppcp open tag.
 * \param user user object.
 * \param to   value of \c to:n attribute (or empty string).
 * \param neg  if \c true \c to:neg will be set to \c "neg".
 */
std::string ppcpOpen(const User &user, const std::string &to, bool neg=false);


/** Returns a \c ppcp close tag (that is <tt>"\</ppcp\>"</tt>). */
inline const std::string &ppcpClose() {
	static const std::string data("</ppcp>");
	return data;
}


/**
 * Returns a \c st element.
 * \param st   user's state.
 * \param msg  user's status message.
 * \param name user's display name.
 */
std::string st(User::State st, const std::string &msg,
               const std::string &name);


/**
 * Returns a \c st element.
 * \param user user to construct \c st element for.
 */
inline std::string st(const User &user) {
	return st(user.status.state, user.status.message,
	          user.name == user.id.nick ? std::string() : user.name);
}


/** Returns a \c rq element. */
inline const std::string &rq() {
	static const std::string data("<rq/>");
	return data;
}


/**
 * Returns a \c m element.
 * \param msg   message's body.
 * \param flags combination of sig::MessageData::MESSAGE and
 *              sig::MessageData::ACTION flags.
 */
std::string m(const std::string &msg, unsigned flags);


/**
 * Returns a \c m element.
 * \param msg   message.
 */
inline std::string m(const sig::MessageData &msg) {
	return m(msg.data, msg.flags);
}


}
}

#endif
