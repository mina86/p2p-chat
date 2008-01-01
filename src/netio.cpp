/** \file
 * Network I/O operations.
 * $Id: netio.cpp,v 1.7 2008/01/01 02:34:50 mina86 Exp $
 */

#include "shared-buffer.hpp"
#include "netio.hpp"


namespace ppc {


TCPSocket *TCPSocket::connect(Address addr) {
	struct sockaddr_in sockaddr;
	const int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
	}

	try {
		addr.toSockaddr(sockaddr);
		while (::connect(fd, (struct sockaddr*)&sockaddr, sizeof sockaddr)<0){
			if (errno != EINTR) throw IOException("connect: ", errno);
		}
		return new TCPSocket(fd, addr);
	}
	catch (...) {
		close(fd);
		throw;
	}
}


std::string TCPSocket::read() {
	int numbytes;

	while ((numbytes = recv(fd, sharedBuffer, sizeof sharedBuffer, 0)) <= 0) {
		if (numbytes == 0) {
			eof = true;
			throw std::string();
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return std::string();
		} else if (errno != EINTR) {
			throw IOException("recv: ", errno);
		}
	}

	return std::string(sharedBuffer, numbytes);
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

	if (addr.port == 0) {
		/* TODO: addr.port may be zero.  Need to get the information at
		   what part we are really listening. */
	}
}


TCPListeningSocket *TCPListeningSocket::bind(Address addr) {
	const int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
	}

	try {
		common_bind_part(fd, addr);
		if (listen(fd, 16) < 0) {
			throw IOException("listen: ", errno);
		}
		return new TCPListeningSocket(fd, addr);
	}
	catch (...) {
		close(fd);
		throw;
	}
}


TCPSocket *TCPListeningSocket::accept() {
	struct sockaddr_in sockaddr;
	socklen_t size = sizeof sockaddr;
	const int new_fd;

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


UDPSocket* UDPSocket::bind(Address addr) {
	const int fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		throw IOException("socket: ", errno);
	}

	try {
		const int t = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof t) < 0) {
			throw IOException("setsockopt: reuseaddr: ", errno);
		}
		common_bind_part(fd, addr);

		if ((addr.ip & 0xf8000000) == 0x08000000) {
			struct ip_mreq mreq;
			mreq.imr_multiaddr.s_addr = addr.ip;
			mreq.imr_interface.s_addr = 0;
			if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			               &mreq, sizeof(mreq)) < 0) {
				throw IOException("setsockopt: add membership: ", errno);
			}
		}

		return new UDPSocket(fd, addr);
	}
	catch (...) {
		close(fd);
		throw;
	}

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

	addr.assign(sockaddr);
	return std::string(sharedBuffer, numbytes);
}


void UDPSocket::write() {
	struct sockaddr_in sockaddr;

	while (!queue.empty()) {
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
		} else if (errno == EINTR) {
			continue;
		} else {
			queue.pop();
			throw IOException("sendto: ", errno);
		}

		queue.pop();
	}
}

}
