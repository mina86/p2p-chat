/** \file
 * Network I/O operations.
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

#ifndef H_IO_HPP
#define H_IO_HPP

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "exception.hpp"


namespace ppc {


/**
 * An exception in input/output operations.
 */
struct IOException : public Exception {
	/**
	 * Constructor.
	 * \param msg error message.
	 */
	IOException(const std::string &msg) : Exception(msg) { }

	/**
	 * Constructor.
	 * \param msg error message prefix.
	 * \param err error numbyer (probably value of \a errno variable).
	 */
	IOException(const std::string &msg, int err)
		: Exception(msg + strerror(err)) { }
};


/**
 * Exception thrown when end of file is encontered.
 */
struct EndOfFile : public IOException {
	/**
	 * Constructor.
	 * \param msg error message.
	 */
	EndOfFile(const std::string &msg = "Unexpected end of file")
		: IOException(msg) { }
};


/**
 * A base class for file descriptors opened in non-blocking mode.
 */
struct FileDescriptor {
	/** File descriptor number. */
	const int fd;

	/**
	 * Closes file descriptor.
	 */
	~FileDescriptor() {
		close(fd);
	}

	/**
	 * Sets file descriptor into non-blocking mode.
	 * \param fd file descriptor.
	 * \throw IOException on error.
	 */
	static void setNonBlocking(int fd) {
		int flags = fcntl(fd, F_GETFL);
		if (flags < 0 ||
		    (!(flags & O_NONBLOCK) &&
		     fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)) {
			throw IOException("fcntl: ", errno);
		}
	}

	/**
	 * Sets file descriptor into non-blocking mode.
	 * \throw IOException on error.
	 */
	void setNonBlocking() {
		setNonBlocking(fd);
	}

protected:
	/**
	 * Constructs object.
	 * \param f file descriptor number.
	 * \param nonBlocking if \c true a \c O_NONBLOCK flag is set on
	 *        this descriptor.
	 */
	FileDescriptor(int f, bool nonBlocking = true) : fd(f) {
		if (nonBlocking) {
			setNonBlocking(f);
		}
	}
};


}

#endif
