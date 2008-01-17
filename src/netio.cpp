/** \file
 * Network I/O operations.
 * $Id: netio.cpp,v 1.16 2008/01/17 17:29:17 mina86 Exp $
 */

#include "shared-buffer.hpp"
#include "netio.hpp"


namespace ppc {


int TCPSocket::connect(Address addr, bool &inProgress) {
	struct sockaddr_in sockaddr;
	const int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
	}

	try {
		FileDescriptor::setNonBlocking(fd);

		inProgress = false;
		addr.toSockaddr(sockaddr);
		while (::connect(fd, (struct sockaddr*)&sockaddr, sizeof sockaddr)<0){
			if (errno == EINPROGRESS) {
				inProgress = true;
				break;
			}
			if (errno != EINTR) throw IOException("connect: ", errno);
		}
	}
	catch (...) {
		close(fd);
		throw;
	}

	return fd;
}


std::string TCPSocket::read() {
	int numbytes;

	while ((numbytes = recv(fd, sharedBuffer, sizeof sharedBuffer, 0)) <= 0) {
		if (numbytes == 0) {
			flags |= 1;
			return std::string();
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return std::string();
		} else if (errno != EINTR) {
			throw IOException("recv: ", errno);
		}
	}

	return std::string(sharedBuffer, numbytes);
}


void TCPSocket::write() {
	int pos, len = data.length(), ret;

	if (flags & 2) {
		socklen_t optlen = sizeof pos;
		ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &pos, &optlen);
		if (ret < 0) {
			throw IOException("getsockopt: ", errno);
		}
		if (pos) {
			throw IOException("connect: ", pos);
		}
		flags &= ~2;
	}

	if (!len) {
		return;
	}

	pos = 0;
	const char *const buf = data.data();
	while (len > 0) {
		ret = send(fd, buf + pos, len, MSG_NOSIGNAL);
		if (ret > 0) {
			pos += ret;
			len -= ret;
		} else if (!ret || errno == EAGAIN || errno == EWOULDBLOCK) {
			break;
		} else if (errno != EINTR) {
			throw IOException("send: ", errno);
		}
	}
	data.erase(0, pos);
}


/**
 * Performs a common part of bind for TCP and UDP sockets.
 * \param fd socket to bind
 * \param addr address to bind, alters it if it's port was zero.
 * \throw IOException on error.
 */
static void common_bind_part(int fd, Address &addr) {
	struct sockaddr_in sockaddr;

	addr.toSockaddr(sockaddr);
	if (bind(fd, (struct sockaddr*)&sockaddr, sizeof sockaddr) < 0) {
		throw IOException("bind: ", errno);
	}

	if (!addr.port) {
		socklen_t addrlen = sizeof sockaddr;
		if (getsockname(fd, (struct sockaddr*)&sockaddr, &addrlen) < 0) {
			throw IOException("getsockname: ", errno);
		}
		addr.port = ntohs(sockaddr.sin_port);
	}
}


std::pair<int, Address> TCPListeningSocket::bind(Address addr) {
	const int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
	}

	try {
		common_bind_part(fd, addr);
		if (listen(fd, 16) < 0) {
			throw IOException("listen: ", errno);
		}
	}
	catch (...) {
		close(fd);
		throw;
	}

	return std::make_pair(fd, addr);
}


TCPSocket *TCPListeningSocket::accept() {
	struct sockaddr_in sockaddr;
	socklen_t size = sizeof sockaddr;
	int new_fd;

	while ((new_fd = ::accept(fd, (struct sockaddr*)&sockaddr, &size)) < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		} else if (errno != ECONNABORTED && errno != EINTR) {
			throw IOException("accept: ", errno);
		}
	}

	try {
		return new TCPSocket(new_fd, Address(sockaddr));
	}
	catch (...) {
		close(new_fd);
		throw;
	}
}


std::pair<int, Address> UDPSocket::bind(Address addr) {
	const int fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
	}

	try {
		const int t = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof t) < 0) {
			throw IOException("setsockopt: reuseaddr: ", errno);
		}

		if (!addr.ip.isMulticast()) {
			common_bind_part(fd, addr);
		} else {
			const int yes = 1;
			struct ip_mreq mreq;
			IP ip(addr.ip);

			addr.ip = 0UL;
			common_bind_part(fd, addr);
			addr.ip = ip;

			if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP,
			               &yes, sizeof yes) < 0) {
				throw IOException("setsockopt: multicast loop: ", errno);
			}

			mreq.imr_multiaddr.s_addr = addr.ip.network();
			mreq.imr_interface.s_addr = 0;
			if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			               &mreq, sizeof(mreq)) < 0) {
				throw IOException("setsockopt: add membership: ", errno);
			}
		}
	}
	catch (...) {
		close(fd);
		throw;
	}

	return std::make_pair(fd, addr);
}


std::string UDPSocket::read(Address &addr) {
	struct sockaddr_in sockaddr;
	socklen_t size = sizeof sockaddr;
	int numbytes;

	while ((numbytes = recvfrom(fd, sharedBuffer, sizeof sharedBuffer, 0,
	                            (struct sockaddr*)&sockaddr, &size)) <= 0) {
		if (numbytes == 0) {
			throw IOException("Unexpected end of file");
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return std::string();
		} else if (errno != EINTR) {
			throw IOException("recvfrom: ", errno);
		}
	}

	addr = sockaddr;
	return std::string(sharedBuffer, numbytes);
}


void UDPSocket::write() {
	struct sockaddr_in sockaddr;

	while (!queue.empty()) {
		std::pair<std::string, Address> &pair = queue.front();
		int ret;

		pair.second.toSockaddr(sockaddr);
		ret = sendto(fd, pair.first.data(), pair.first.size(), 0,
		             (struct sockaddr*)&sockaddr, sizeof sockaddr);
		if (ret > 0) {
			/* ok, we sent something -- we don't really know if it was
			   a whole datagram but lets hope it was */
			queue.pop();
		} else if (ret == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		} else if (errno == EMSGSIZE) {
			/* message too long -- so what can we do about it? */
			queue.pop();
		} else if (errno != EINTR) {
			queue.pop();
			throw IOException("sendto: ", errno);
		}
	}
}

}
