/** \file
 * Network I/O operations.
 * $Id: netio.hpp,v 1.10 2008/01/03 18:39:10 mina86 Exp $
 */

#ifndef H_NETIO_HPP
#define H_NETIO_HPP

#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <string>

#include "vector-queue.hpp"
#include "shared-buffer.hpp"
#include "io.hpp"


namespace ppc {


/** Type representing IP address. */
struct IP {
	/**
	 * Sets IP address from address in host byte order.
	 * \param val IP address in host byte order.
	 */
	IP(unsigned long val = 0) : value(val) { }

	/**
	 * Sets IP address from \c in_addr structure.
	 * \param addr \c in_addr structure holding ip address in network
	 *             byte order.
	 */
	IP(struct in_addr addr) : value(ntoh(addr.s_addr)) { }

	/**
	 * Sets IP address from \c sockaddr_in structure.
	 * \param addr \c sockaddr_in structure holding ip address in network
	 *             byte order.
	 */
	IP(const struct sockaddr_in &addr) : value(ntoh(addr.sin_addr.s_addr)) { }


	/** Returns \c true if IP address is a multicast address (class D). */
	bool isMulticast() const {
		return value & 0xf8000000 == 0x08000000;
	}


	/** Returns IP address in host byte order. */
	unsigned long host     () const { return      value ; }

	/** Returns IP address in network byte order. */
	unsigned long network  () const { return hton(value); }

	/** Returns IP address in host byte order. */
	operator unsigned long () const { return      value ; }

	/** Returns IP address in network byte order in \c in_addr structure. */
	operator struct in_addr() const {
		struct in_addr addr;
		addr.s_addr = network();
		return addr;
	}


	/**
	 * Sets IP address from address in host byte order.
	 * \param val IP address in host byte order.
	 */
	IP &operator=(unsigned long val) {
		value = val;
		return *this;
	}

	/**
	 * Sets IP address from \c in_addr structure.
	 * \param addr \c in_addr structure holding ip address in network
	 *             byte order.
	 */
	IP &operator=(struct in_addr addr) {
		value = ntoh(addr.s_addr);
		return *this;
	}

	/**
	 * Sets IP address from \c sockaddr_in structure.
	 * \param addr \c sockaddr_in structure holding ip address in network
	 *             byte order.
	 */
	IP &operator=(struct sockaddr_in addr) {
		value = ntoh(addr.sin_addr.s_addr);
		return *this;
	}

	/**
	 * Convers IP address in network byte order to host byte order.
	 * \param val IP address in network byte order.
	 */
	static unsigned long ntoh(unsigned long val) { return ntohl(val); }

	/**
	 * Convers IP address in host byte order to network byte order.
	 * \param val IP address in host byte order.
	 */
	static unsigned long hton(unsigned long val) { return htonl(val); }

private:
	/** IP address in host byte order. */
	unsigned long value;
};


/** Type representing TCP/UDP port number. */
struct Port {
	/**
	 * Sets port number from numer in host byte order.
	 * \param val port number in host byte order.
	 */
	Port(unsigned short val = 0) : value(val) { }

	/**
	 * Sets IP address from \c sockaddr_in structure.
	 * \param addr \c sockaddr_in structure holding port number in
	 *             network byte order.
	 */
	Port(struct sockaddr_in addr) : value(ntoh(addr.sin_port)) { }


	/** Returns port number in host byte order. */
	unsigned short host     () const { return      value ; }

	/** Returns port number in network byte order. */
	unsigned short network  () const { return hton(value); }

	/** Returns port number in host byte order. */
	operator unsigned short () const { return      value ; }


	/**
	 * Sets port number from numer in host byte order.
	 * \param val port number in host byte order.
	 */
	Port &operator=(unsigned short val) {
		value = val;
		return *this;
	}

	/**
	 * Sets IP address from \c sockaddr_in structure.
	 * \param addr \c sockaddr_in structure holding port number in
	 *             network byte order.
	 */
	Port &operator=(struct sockaddr_in addr) {
		value = ntoh(addr.sin_port);
		return *this;
	}


	/**
	 * Convers port number in network byte order to host byte order.
	 * \param val port number in network byte order.
	 */
	static unsigned short ntoh(unsigned short val) { return ntohs(val); }

	/**
	 * Convers port number in host byte order to network byte order.
	 * \param val port number in host byte order.
	 */
	static unsigned short hton(unsigned short val) { return htons(val); }

private:
	/** Port number in host byte order. */
	unsigned short value;
};


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
	explicit Address(const struct sockaddr_in &addr) : ip(addr), port(addr) {}


	/**
	 * Copies values from a sockaddr_in structure.
	 * \param addr address.
	 */
	Address &operator=(const struct sockaddr_in &addr) {
		ip = addr;
		port = addr;
		return *this;
	}


	/** Returns Address as a string. */
	std::string toString() const {
		int ret = sprintf(sharedBuffer, "%s:%u", inet_ntoa(ip), port.host());
		return std::string(sharedBuffer, ret);
	}


	/**
	 * Fills a sockaddr_in structure.
	 * \param addr sockaddr_in structure to fill in.
	 */
	void toSockaddr(struct sockaddr_in &addr) {
		addr.sin_family = AF_INET;
		addr.sin_port = port.network();
		addr.sin_addr.s_addr = ip.network();
		memset(addr.sin_zero, 0, sizeof addr.sin_zero);
	}
};



/** Base class for TCP and UDP sockets. */
struct Socket : public FileDescriptor {
	/**
	 * Socket address.  Depending on socket's type this may be either
	 * address it is connected to or bound to.
	 */
	const Address address;

protected:
	/**
	 * Creates socket by setting file descriptor and address.  File
	 * descriptor's O_NONBLOCK flag is set.
	 *
	 * \param sock socket's file descriptor number.
	 * \param addr address associated with socket.
	 * \param nonBlocking if \c true a \c O_NONBLOCK flag is set on
	 *        this descriptor (idea behind this argument is that if
	 *        socket is already in non-blocking mode this mode does
	 *        not need to be set again).
	 */
	Socket(int sock, Address addr, bool nonBlocking = true)
		: FileDescriptor(sock, nonBlocking), address(addr) {}

	/**
	 * Creates socket by setting file descriptor and address.  File
	 * descriptor's O_NONBLOCK flag is set.
	 *
	 * \param info socket's file descriptor number and address.
	 * \param nonBlocking if \c true a \c O_NONBLOCK flag is set on
	 *        this descriptor.
	 */
	explicit Socket(std::pair<int, Address> info, bool nonBlocking = true)
		: FileDescriptor(info.first, nonBlocking), address(info.second) {}
};



/** A TCP socket. */
struct TCPSocket : public Socket {
	/**
	 * Creats new TCP socket and connects to given address.
	 *
	 * \param addr  address to connect to.
	 * \param dummy just a dirty hack, don't worry about this argument
	 *              it does nothing.
	 * \throw IOException if error occured.
	 */
	explicit TCPSocket(Address addr, bool dummy = false) :
		/* dirty hack -- dummy will be set to value of inProgress */
		Socket(connect(addr, dummy), addr, false) {
		flags = dummy ? 2 : 0;
	}

	/**
	 * Pushes data to buffer to send it later on.
	 * \param str string to append to buffer.
	 */
	void push(const std::string &str) { data += str; }

	/** Returns whether there is any data to send. */
	bool hasDataToWrite() {
		/* When INPROGRESS flag is set we don't necceserly have any
		   data to write but anyhow we need to poll descriptor for
		   writing to get error code when connection is established or
		   not. */
		return (flags & 2) || !data.empty();
	}

	/**
	 * Reads data from socket.  This method must not block!  If there
	 * is no data ready to be read method shall return empty string.
	 * \return read string.
	 * \throw IOException if error occured.
	 */
	std::string read();

	/**
	 * Writes buffered data to socket.  This method must not block!
	 * It should write us much data as it can without blocking.  It
	 * shall return when there is no more data pending to be sent or
	 * write operation would block.
	 *
	 * \throw IOException if error occured.
	 */
	void write();

	/** Returns value of end of file flag. */
	bool isEOF() const {
		return flags & 1;
	}


private:
	/** Buffered data to send. */
	std::string data;

	/** Socket flags. */
	unsigned char flags;

	/**
	 * Creates TCP socket and connects to given address.
	 * \param addr address to connected to.
	 * \param inProgress output variable which is set to \c true if
	 *                   connecting is in progress and to \c false
	 *                   otherwise (if method throws an exception
	 *                   value is unspecified).
	 * \return socket file descriptor number.
	 * \throw IOException if error occured.
	 */
	static int connect(Address addr, bool &inProgress);

	/**
	 * Initialises TCPSocket.
	 * \param sock socket number.
	 * \param addr address we are connected to.
	 * \param nonBlocking if \c true a \c O_NONBLOCK flag is set on
	 *        this descriptor.
	 */
	TCPSocket(int sock, Address addr, bool nonBlocking = true) :
		Socket(sock, addr, nonBlocking), flags(0) { }

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
	 * can bind to any port number.
	 *
	 * \param addr address to bind to.
	 * \throw IOException if error occured.
	 */
	TCPListeningSocket(Address addr) : Socket(bind(addr), true) { };


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
	 * \throw IOException if error occured.
	 */
	TCPSocket *accept();

private:
	/**
	 * Creats new TCP listening socket and binds to given address.  IP
	 * address withing \a addr may be zero which means to bind to all
	 * IP addresses.  Also port number may be zero which means that we
	 * can bind to any port number.
	 *
	 * \param addr address to bind to.
	 * \return a socket file descriptor number and address socket is
	 *         bound to pair (IP address of returned address may be
	 *         zero but port number will never be zero).
	 * \throw IOException if error occured.
	 */
	static std::pair<int, Address> bind(Address addr);
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
	 * \param nonBlocking if \c true a \c O_NONBLOCK flag is set on
	 *        this descriptor.
	 * \throw IOException if error occured.
	 */
	UDPSocket(Address addr) : Socket(bind(addr), true) { };


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
	 * \throw IOException if error occured.
	 */
	std::string read(Address &addr);

	/**
	 * Writes buffered data to socket.  This method must not block!
	 * It should write us much data as it can without blocking.  It
	 * shall return when there is no more data pending to be sent or
	 * write operation would block.
	 *
	 * \throw IOException if error occured.
	 */
	void write();


private:
	/** Queue of datagrams to send. */
	std::queue< std::pair<std::string, Address>,
	            std::vector< std::pair<std::string, Address> > > queue;


	/**
	 * Creats new UDP socket.  If \a addr is not a zero address (that
	 * is address having both IP and port number zero) also binds
	 * socket to given address.  IP address may be a broadcast or
	 * multicast address in which case method takes apropriate actions
	 * required by given class of addresses.
	 *
	 * \param addr address to bind to.
	 * \return a socket file descriptor number and address socket is
	 *         bound to pair (IP address of returned address may be
	 *         zero but port number will never be zero).
	 * \throw IOException if error occured.
	 */
	static std::pair<int, Address> bind(Address addr);
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
