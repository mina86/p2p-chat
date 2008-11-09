/** \file
 * Base exception class.
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

#ifndef H_EXCEPTION_HPP
#define H_EXCEPTION_HPP

#include <string>

namespace ppc {


/**
 * Abstract class that all other exceptions should inherit from.
 */
struct Exception {
	/**
	 * Constructor.
	 * \param msg error message.
	 */
	Exception(const std::string &msg) : message(msg) { }

	/** Returns error message. */
	const std::string &getMessage() const {
		return message;
	}

private:
	/** Error message. */
	std::string message;
};


}
#endif
