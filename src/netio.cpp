/** \file
 * Network I/O operations.
 * $Id: netio.cpp,v 1.2 2007/12/29 22:24:33 mina86 Exp $
 */

#include "shared-buffer.hpp"
#include "netio.hpp"


namespace ppc {




TCPSocket *TCPSocket::connect(Address addr) {
	struct sockaddr_in sockaddr;
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw NetException(std::string("socket: ") + strerror(errno));
	}

	addr.toSockaddr(sockaddr);
	if (::connect(fd, (struct sockaddr*)&sockaddr, sizeof sockaddr) < 0) {
		throw NetException(std::string("connect: ") + strerror(errno));
	}

	return new TCPSocket(fd, addr);
}


std::string TCPSocket::read() {
	int numbytes = recv(fd, sharedBuffer, sizeof sharedBuffer, 0);
	if (numbytes > 0) {
		return std::string(sharedBuffer, numbytes);
	} else if (numbytes == 0) {
		/* TODO: end of file condition; for now throw an error until
		   we think of any other sollution */
		throw NetException("end of stream");
	} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return std::string();
	} else {
		throw NetException(std::string("recv: ") + strerror(errno));
	}
}


void TCPSocket::write() {
	int pos = 0, len = data.length(), ret;
	const char *buf = data.data();
	while (len > 0) {
		ret = send(fd, buf + pos, len, 0);
		if (ret > 0) {
			pos += ret;
			len -= ret;
		} else if (!ret || errno == EAGAIN || errno == EWOULDBLOCK) {
			break;
		} else {
			throw NetException(std::string("send: ") + strerror(errno));
		}
	}
	data.erase(0, pos);
}


/**
 * Performs a common part of bind for TCP and UDP sockets.
 * \param fd socket to bind
 * \param addr address to bind, alters it if it's port was zero.
 * \thorw NetException on error.
 */
static void common_bind_part(int fd, Address &addr) {
	struct sockaddr_in sockaddr;

	addr.toSockaddr(sockaddr);
	if (bind(fd, (struct sockaddr*)&sockaddr, sizeof sockaddr) < 0) {
		throw NetException(std::string("bind: ") + strerror(errno));
	}

	if (addr.port == 0) {
		/* TODO: addr.port may be zero.  Need to get the information at
		   what part we are really listening. */
	}
}


TCPListeningSocket *TCPListeningSocket::bind(Address addr) {
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw NetException(std::string("socket: ") + strerror(errno));
	}

	common_bind_part(fd, addr);

	if (listen(fd, 16) < 0) {
		throw NetException(std::string("listen: ") + strerror(errno));
	}

	return new TCPListeningSocket(fd, addr);
}


TCPSocket *TCPListeningSocket::accept() {
	struct sockaddr_in sockaddr;
	socklen_t size = sizeof sockaddr;
	int new_fd = ::accept(fd, (struct sockaddr*)&sockaddr, &size);

	if (new_fd < 0) {
		if (errno == EAGAIN || errno==EWOULDBLOCK) {
			return 0;
		} else {
			throw NetException(std::string("accept: ") + strerror(errno));
		}
	}

	return new TCPSocket(new_fd, Address(sockaddr));
}


UDPSocket* UDPSocket::bind(Address addr) {
	int fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		throw NetException(std::string("socket: ") + strerror(errno));
	}

	common_bind_part(fd, addr);

	return new UDPSocket(fd, addr);
}


std::string UDPSocket::read(Address &addr) {
	struct sockaddr_in sockaddr;
	socklen_t size = sizeof sockaddr;
	int numbytes = recvfrom(fd, sharedBuffer, sizeof sharedBuffer, 0,
	                        (struct sockaddr*)&sockaddr, &size);

	if (numbytes > 0) {
		addr.assign(sockaddr);
		return std::string(sharedBuffer, numbytes);
	} else if (numbytes == 0) {
		throw NetException("recvfrom: returned 0");
	} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return std::string();
	} else {
		throw NetException(std::string("recvfrom: ") + strerror(errno));
	}
}


void UDPSocket::write() {
	struct sockaddr_in sockaddr;

	while(!queue.empty()) {
		std::pair<std::string, Address> &pair = queue.front();
		int ret;

		pair.second.toSockaddr(sockaddr);
		ret = sendto(fd, pair.first.data(), pair.first.size(), 0,
		             (struct sockaddr*)&sockaddr, sizeof sockaddr);
		if (ret > 0) {
			/* ok, we sent something -- we don't really know if it was
			   a whole datagram but lets hope it was */
		} else if (ret == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		} else {
			queue.pop();
			throw NetException(std::string("sendto: ") + strerror(errno));
		}

		queue.pop();
	}
}

}
