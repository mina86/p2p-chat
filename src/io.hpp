/** \file
 * Network I/O operations.
 * $Id: io.hpp,v 1.1 2007/12/30 15:14:44 mina86 Exp $
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
struct NonBlockingFD {
	/** File descriptor number. */
	const int fd;

	/**
	 * Closes file descriptor.
	 */
	~NonBlockingFD() {
		close(fd);
	}

	/**
	 * Sets file descriptor into non-blocking mode.
	 * \param fd file descriptor.
	 * \param IOException on error.
	 */
	static void setNonBlocking(int fd) {
		int flags = fcntl(fd, F_GETFL);
		if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
			throw IOException("fcntl: ", errno);
		}
	}

protected:
	/**
	 * Constructs object.
	 * \param f file descriptor number
	 */
	NonBlockingFD(int f) : fd(f) {
		setNonBlocking(f);
	}
};


}

#endif
