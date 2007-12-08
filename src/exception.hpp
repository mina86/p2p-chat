/** \file
 * Base exception class.
 * $Id: exception.hpp,v 1.1 2007/12/08 18:02:53 mina86 Exp $
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
