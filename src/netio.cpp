/** \file
 * Network I/O operations.
 * $Id: netio.cpp,v 1.4 2007/12/30 15:19:49 mina86 Exp $
 */

#include "shared-buffer.hpp"
#include "netio.hpp"


namespace ppc {


TCPSocket *TCPSocket::connect(Address addr) {
	struct sockaddr_in sockaddr;
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
	}

	addr.toSockaddr(sockaddr);
	if (::connect(fd, (struct sockaddr*)&sockaddr, sizeof sockaddr) < 0) {
		throw IOException("connect: ", errno);
	}

	return new TCPSocket(fd, addr);
}


std::string TCPSocket::read() {
	int numbytes = recv(fd, sharedBuffer, sizeof sharedBuffer, 0);
	if (numbytes > 0) {
		return std::string(sharedBuffer, numbytes);
	} else if (numbytes == 0) {
		eof = true;
		throw std::string();
	} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return std::string();
	} else {
		throw IOException("recv: ", errno);
	}
}


void TCPSocket::write() {
	int pos = 0, len = data.length(), ret;
	const char *buf = data.data();
	while (len > 0) {
		ret = send(fd, buf + pos, len, MSG_NOSIGNAL);
		if (ret > 0) {
			pos += ret;
			len -= ret;
		} else if (!ret || errno == EAGAIN || errno == EWOULDBLOCK) {
			break;
		} else {
			throw IOException("send: ", errno);
		}
	}
	data.erase(0, pos);
}


/**
 * Performs a common part of bind for TCP and UDP sockets.
 * \param fd socket to bind
 * \param addr address to bind, alters it if it's port was zero.
 * \thorw IOException on error.
 */
static void common_bind_part(int fd, Address &addr) {
	struct sockaddr_in sockaddr;

	addr.toSockaddr(sockaddr);
	if (bind(fd, (struct sockaddr*)&sockaddr, sizeof sockaddr) < 0) {
		throw IOException("bind: ", errno);
	}

	if (addr.port == 0) {
		/* TODO: addr.port may be zero.  Need to get the information at
		   what part we are really listening. */
	}
}


TCPListeningSocket *TCPListeningSocket::bind(Address addr) {
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
	}

	common_bind_part(fd, addr);

	if (listen(fd, 16) < 0) {
		throw IOException("listen: ", errno);
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
			throw IOException("accept: ", errno);
		}
	}

	return new TCPSocket(new_fd, Address(sockaddr));
}


UDPSocket* UDPSocket::bind(Address addr) {
	int fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
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
		throw IOException("Unexpected end of file");
	} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return std::string();
	} else {
		throw IOException("recvfrom: ", errno);
	}
}


void UDPSocket::write() {
	struct sockaddr_in sockaddr;

	while(!queue.empty()) {
		std::pair<std::string, Address> &pair = queue.front();
		int ret;

		pair.second.toSockaddr(sockaddr);
		ret = sendto(fd, pair.first.data(), pair.first.size(), MSG_NOSIGNAL,
		             (struct sockaddr*)&sockaddr, sizeof sockaddr);
		if (ret > 0) {
			/* ok, we sent something -- we don't really know if it was
			   a whole datagram but lets hope it was */
		} else if (ret == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		} else {
			queue.pop();
			throw IOException("sendto: ", errno);
		}

		queue.pop();
	}
}

}
