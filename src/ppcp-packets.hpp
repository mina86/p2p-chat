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
