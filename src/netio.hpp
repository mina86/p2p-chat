/** \file
 * Network I/O operations.
 * $Id: netio.hpp,v 1.6 2007/12/29 22:24:17 mina86 Exp $
 */

#ifndef H_NETIO_HPP
#define H_NETIO_HPP

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <string>

#include "vector-queue.hpp"
#include "shared-buffer.hpp"
#include "exception.hpp"


namespace ppc {


/**
 * An exception in network communication.
 */
struct NetException : public Exception {
	/**
	 * Constructor.
	 * \param msg error message.
	 */
	NetException(const std::string &msg) : Exception(msg) { }
};


/** Type representing IP address. */
typedef unsigned long IP;

/** Type representing TCP/UDP port number. */
typedef unsigned short Port;

/** A network half-association (IP address, port number). */
struct Address {
	/** IP address. */
	IP ip;
	/** Port number. */
	Port port;

	/**
	 * Default constructor.
	 * \param i IP address.
	 * \param p port number.
	 */
	explicit Address(IP i = 0, Port p = 0) : ip(i), port(p) { }

	/**
	 * Constructs Address from a sockaddr_in structure.
	 * \param addr address.
	 */
	explicit Address(const struct sockaddr_in &addr)
		: ip(ntohl(addr.sin_addr.s_addr)), port(ntohs(addr.sin_port)) { }


	/**
	 * Copies values from a sockaddr_in structure.
	 * \param addr address.
	 */
	void assign(const struct sockaddr_in &addr) {
		ip = ntohl(addr.sin_addr.s_addr);
		port = ntohs(addr.sin_port);
	}


	/** Returns Address as a string. */
	std::string toString() const {
		sprintf(sharedBuffer, "%lu:%u", ip, port);
		return sharedBuffer;
	}


	/**
	 * Fills a sockaddr_in structure.
	 * \param addr sockaddr_in structure to fill in.
	 */
	void toSockaddr(struct sockaddr_in &addr) {
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htons(ip);
		memset(addr.sin_zero, 0, sizeof addr.sin_zero);
	}
};



/** Base class for TCP and UDP sockets. */
struct Socket {
	/** Closes socket. */
	~Socket() {
		close(fd);
	}

	/** Returns socket number. */
	int getFD() { return fd; }

	/**
	 * Returns address associated with socket.  Depending on socket's
	 * type this may be either address it is connected to or bound
	 * to.
	 */
	Address getAddress() const { return address; }

protected:
	/** Socket number. */
	int fd;

	/** Socket address. */
	Address address;

	/**
	 * Creates socket by setting file descriptor and address.
	 * \param sock socket's file descriptor number.
	 * \param addr address associated with socket.
	 */
	Socket(int sock, Address addr) : fd(sock), address(addr) {
		int flags = fcntl(sock, F_GETFL);
		if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
			throw NetException(std::string("fcntl: ") + strerror(errno));
		}
	}
};



/** A TCP socket. */
struct TCPSocket : public Socket {
	/**
	 * Creats new TCP socket and connects to given address.
	 *
	 * \param addr address to connect to.
	 * \return new TCP socket.
	 * \throw NetException if error occured.
	 */
	static TCPSocket *connect(Address addr);


	/**
	 * Pushes data to buffer to send it later on.
	 * \param str string to append to buffer.
	 */
	void push(const std::string &str) { data += str; }

	/** Returns whether there is any data to send. */
	bool hasDataToWrite() { return !data.empty(); }

	/**
	 * Reads data from socket.  This method must not block!  If there
	 * is no data ready to be read method shall return empty string.
	 * \return read string.
	 * \throw NetException if error occured.
	 */
	std::string read();

	/**
	 * Writes buffered data to socket.  This method must not block!
	 * It should write us much data as it can without blocking.  It
	 * shall return when there is no more data pending to be sent or
	 * write operation would block.
	 *
	 * \throw NetException if error occured.
	 */
	void write();


private:
	/** Buffered data to send. */
	std::string data;

	/**
	 * Initialises TCPSocket.
	 * \param sock socket number.
	 * \param addr address we are connected to.
	 */
	TCPSocket(int sock, Address addr) : Socket(sock, addr) { }

	/* TCPListeningSocket needs to create TCPSocket objects when it
	   accepts connection */
	friend struct TCPListeningSocket;
};



/** A TCP Listening socket. */
struct TCPListeningSocket : public Socket {
	/**
	 * Creats new TCP listening socket and binds to given address.  IP
	 * address withing \a addr may be zero which means to bind to all
	 * IP addresses.  Also port number may be zero which means that we
	 * can bind to any port number.  If port was zero method must
	 * later fill it with accual port number.
	 *
	 * \param addr address to bind to.
	 * \return new TCP listening socket.
	 * \throw NetException if error occured.
	 */
	static TCPListeningSocket *bind(Address addr);


	/**
	 * Accepts new connection.  Method must not block! If there are no
	 * more pending connections \c 0 shall be returned otherwise
	 * a TCPSocket representing accepted connection.
	 *
	 * Method handles cases when a connection has been aborted or
	 * system call was interrupted by signal.  In those situations
	 * exception is no thrown.
	 *
	 * \return accepted TCP connection or \c 0.
	 * \throw NetException if error occured.
	 */
	TCPSocket *accept();


private:
	/**
	 * Initialises TCPListeningSocket.
	 * \param sock socket number.
	 * \param addr address we are bound to.
	 */
	TCPListeningSocket(int sock, Address addr): Socket(sock, addr) {}
};



/** An UDP socket. */
struct UDPSocket : public Socket {
	/**
	 * Creats new UDP socket.  If \a addr is not a zero address (that
	 * is address having both IP and port number zero) also binds
	 * socket to given address.  IP address may be a broadcast or
	 * multicast address in which case method takes apropriate actions
	 * required by given class of addresses.
	 *
	 * \param addr address to bind to.
	 * \return new UDP socket.
	 * \throw NetException if error occured.
	 */
	static UDPSocket *bind(Address addr);


	/**
	 * Pushes data to buffer to send it later on.
	 * \param str string to append to buffer.
	 * \param addr address to send data to.
	 */
	void push(const std::string &str, Address addr) {
		queue.push(std::pair<std::string, Address>(str, addr));
	}

	/** Returns whether there is any data to send. */
	bool hasDataToWrite() { return !queue.empty(); }

	/**
	 * Reads data from socket.  This method must not block!  If there
	 * is no data ready to be read method shall return empty string.
	 *
	 * \param addr address datagram was recieved from.
	 * \return read string.
	 * \throw NetException if error occured.
	 */
	std::string read(Address &addr);

	/**
	 * Writes buffered data to socket.  This method must not block!
	 * It should write us much data as it can without blocking.  It
	 * shall return when there is no more data pending to be sent or
	 * write operation would block.
	 *
	 * \throw NetException if error occured.
	 */
	void write();


private:
	/** Queue of datagrams to send. */
	std::queue< std::pair<std::string, Address>,
	            std::vector< std::pair<std::string, Address> > > queue;

	/**
	 * Initialises UDPSocket.
	 * \param sock socket number.
	 * \param addr address we are bound to.
	 */
	UDPSocket(int sock, Address addr): Socket(sock, addr), queue() {}
};





/**
 * Returns \c true if Address objects are equal.  Objects are equal
 * if they have the same IP address and port number.
 * \param a first Address object to compare.
 * \param b second Address object to compare.
 */
inline bool operator==(const Address &a, const Address &b) {
	return a.ip == b.ip && a.port == b.port;
}


/**
 * Returns \c true if Address objects are not equal.  Objects are
 * equal if they have the same IP address and port number.
 * \param a first Address object to compare.
 * \param b second Address object to compare.
 */
inline bool operator!=(const Address &a, const Address &b) {
	return !(a == b);
}


/**
 * Address linear order.  This order is defined as follows: <tt>a <=
 * b iff (a.ip, a.port) <= (b.ip, b.port)</tt>.
 *
 * \param a first Address object to compare.
 * \param b second Address object to compare.
 * \return \c true if first object is greater then the second.
 */
inline bool operator> (const Address &a, const Address &b) {
	return a.ip > b.ip || (a.ip == b.ip && a.port > b.port);
}


/**
 * Address linear order.  This order is defined as follows: <tt>a <=
 * b iff (a.ip, a.port) <= (b.ip, b.port)</tt>.
 *
 * \param a first Address object to compare.
 * \param b second Address object to compare.
 * \return \c true if first object is greater then or equal to the second.
 */
inline bool operator>=(const Address &a, const Address &b) {
	return a.ip > b.ip || (a.ip == b.ip && a.port >= b.port);
}


/**
 * Address linear order.  This order is defined as follows: <tt>a <=
 * b iff (a.ip, a.port) <= (b.ip, b.port)</tt>.
 *
 * \param a first Address object to compare.
 * \param b second Address object to compare.
 * \return \c true if first object is less then the second.
 */
inline bool operator< (const Address &a, const Address &b) {
	return b > a;
}


/**
 * Address linear order.  This order is defined as follows: <tt>a <=
 * b iff (a.ip, a.port) <= (b.ip, b.port)</tt>.
 *
 * \param a first Address object to compare.
 * \param b second Address object to compare.
 * \return \c true if first object is less then or equal to the second.
 */
inline bool operator<=(const Address &a, const Address &b) {
	return b >= a;
}


}

#endif
