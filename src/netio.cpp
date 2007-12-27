/** \file
 * Network I/O operations.
 * $Id: netio.cpp,v 1.1 2007/12/27 17:41:59 mina86 Exp $
 */

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "netio.hpp"


namespace ppc {


Socket::Socket(int sock, Address addr) : fd(sock), address(addr) {
	int flags = fcntl(sock, F_GETFL);
	if (flags < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		throw NetException(std::string("fcntl: ") + strerror(errno));
	}
}

TCPSocket *TCPSocket::connect(Address addr) {
	(void)addr;
	throw NetException("TCPSocket::connect() not yet implemented");
}


std::string TCPSocket::read() {
	throw NetException("TCPSocket::read() not yet implemented");
}


void TCPSocket::write() {
	throw NetException("TCPSocket::write() not yet implemented");
}



TCPListeningSocket *TCPListeningSocket::bind(Address addr) {
	(void)addr;
	throw NetException("TCPListeningSocket::bind() not yet implemented");
}


TCPSocket *TCPListeningSocket::accept() {
	throw NetException("TCPListeningSocket::accept() not yet implemented");
}



UDPSocket *UDPSocket::bind(Address addr) {
	(void)addr;
	throw NetException("UDPSocket::bind() not yet implemented");
}

std::string UDPSocket::read(Address &addr) {
	(void)addr;
	throw NetException("UDPSocket::read() not yet implemented");
}


void UDPSocket::write() {
	throw NetException("UDPSocket::write() not yet implemented");
}


}
