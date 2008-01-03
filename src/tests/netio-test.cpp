/** \file
 * A netio library test.
 * $Id: netio-test.cpp,v 1.2 2008/01/03 18:44:23 mina86 Exp $
 */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <vector>

#include "../netio.hpp"
#include "../shared-buffer.hpp"


namespace test {


struct Clients {
	typedef std::vector<ppc::TCPSocket*>::iterator iterator;

	~Clients() {
		for (iterator it = begin(), e = end(); it != e; ++it) {
			delete *it;
		}
		sockets.clear();
	}


	void add(ppc::TCPSocket *socket) {
		sockets.push_back(socket);
	}

	void remove(ppc::TCPSocket *socket) {
		for (iterator it = begin(), e = end(); it != e; ++it) {
			if (*it == socket) remove(it);
		}
	}

	iterator remove(iterator it) {
		delete *it;
		return sockets.erase(it);
	}


	iterator begin() { return sockets.begin(); }
	iterator end() { return sockets.end(); }


	void push(const std::string &data) {
		std::cout << "Pushing " << data;
		for (iterator it = begin(), e = end(); it != e; ++it) {
			(*it)->push(data);
		}
	}

	int fillFDSets(fd_set *rd, fd_set *wr) {
		int max = -1;
		for (iterator it = begin(), e = end(); it != e; ++it) {
			const int fd = (*it)->fd;
			if (fd > max) max = fd;
			FD_SET(fd, rd);
			if ((*it)->hasDataToWrite()) FD_SET(fd, wr);
		}
		return max;
	}


private:
	std::vector<ppc::TCPSocket*> sockets;
};


std::ostream &operator<<(std::ostream &out, ppc::Address addr) {
	return out << addr.toString();
}


int server(ppc::Port port) {
	ppc::TCPListeningSocket listener(ppc::Address(0, port));
	Clients clients;
	int ret = 0;

	for(;;) {
		int nfds;
		fd_set rd, wr;

		/* Fill sets */
		FD_ZERO(&rd);
		FD_ZERO(&wr);
		nfds = clients.fillFDSets(&rd, &wr);
		FD_SET(0, &rd);
		FD_SET(listener.fd, &rd);
		if (listener.fd > nfds) nfds = listener.fd;

		/* Select descriptors */
		std::cout << "... selecting\n";
		nfds = select(nfds + 1, &rd, &wr, 0, 0);
		if (nfds == 0) {
			std::cerr << "select: returned 0\n";
			ret = 1;
			break;
		} else if (nfds == -1 && errno == EINTR) {
			break;
		} else if (nfds == -1) {
			std::cerr << "select: " << strerror(errno) << '\n';
			ret = 1;
			break;
		}

		/* Read from stdin */
		if (FD_ISSET(0, &rd)) {
			std::cout << "... reading from stdin\n";
			int num = read(0, ppc::sharedBuffer, sizeof ppc::sharedBuffer);
			if (num < 0) {
				ret = 1;
				perror("read");
				break;
			}
			if (!num) break;
			clients.push(std::string(ppc::sharedBuffer, num));
			--nfds;
		}

		/* Accept connections */
		if(FD_ISSET(listener.fd, &rd)) {
			ppc::TCPSocket *sock;
			std::cout << "... accepting connections\n";
			while ((sock = listener.accept())) {
				std::cout << "New connection from " << sock->address << '\n';
				clients.add(sock);
			}
			--nfds;
		}

		/* Handle other sockets */
		for (Clients::iterator it = clients.begin(), end = clients.end();
		     nfds > 0 && it != end; ++it) {
			if (FD_ISSET((*it)->fd, &wr)) {
				std::cout << "... writing to " << (*it)->address << '\n';
				--nfds;
				(*it)->write();
			}

			if (FD_ISSET((*it)->fd, &rd)) {
				std::string data;
				std::cout << "... reading from " << (*it)->address << '\n';
				--nfds;
				while (!(data = (*it)->read()).empty()) {
					std::cout << "Got: " << data;
					clients.push(data);
				}
				if ((*it)->isEOF()) {
					std::cout << "EOF\n";
					it = clients.remove(it);
					end = clients.end();
				}
			}
		}
	}

	return ret;
}



int client(ppc::IP ip, ppc::Port port) {
	ppc::TCPSocket sock(ppc::Address(ip, port));
	int ret = 0;

	for(;;) {
		int nfds;
		fd_set rd, wr;

		FD_ZERO(&rd);
		FD_ZERO(&wr);
		FD_SET(0, &rd);
		FD_SET(sock.fd, &rd);
		if (sock.hasDataToWrite()) {
			FD_SET(sock.fd, &wr);
		}

		std::cout << "... selecting\n";
		nfds = select(sock.fd + 1, &rd, &wr, 0, 0);
		if (nfds == 0) {
			std::cerr << "select: returned 0\n";
			ret = 1;
			break;
		} else if (nfds == -1 && errno == EINTR) {
			break;
		} else if (nfds == -1) {
			std::cerr << "select: " << strerror(errno) << '\n';
			ret = 1;
			break;
		}

		/* Read from stdin */
		if (FD_ISSET(0, &rd)) {
			std::cout << "... reading from stdin\n";
			int num = read(0, ppc::sharedBuffer, sizeof ppc::sharedBuffer);
			if (num < 0) {
				ret = 1;
				perror("read");
				break;
			}
			if (!num) break;
			std::cout << "Pushing " << std::string(ppc::sharedBuffer, num);
			sock.push(std::string(ppc::sharedBuffer, num));
		}

		/* Read from socket */
		if (FD_ISSET(sock.fd, &rd)) {
			std::string data;
			std::cout << "... reading from " << sock.address << '\n';
			while (!(data = sock.read()).empty()) {
				std::cout << "Got: " << data;
			}
			if (sock.isEOF()) {
				std::cout << "EOF\n";
				break;
			}
		}

		/* Write to socket */
		if (FD_ISSET(sock.fd, &wr)) {
			std::cout << "... writing to " << sock.address << '\n';
			sock.write();
		}
	}

	return ret;
}


}


int main(int argc, char **argv) {
	ppc::Port port = 8888;
	int opt;

	while ((opt = getopt(argc, argv, "p:"))!=EOF) {
		switch (opt) {
		case 'p': port = atoi(optarg); break;
		default: return 1;
		}
	}

	try {
		return optind != argc
			? test::client(ppc::IP::ntoh(inet_addr(argv[optind])), port)
			: test::server(port);
	}
	catch (const ppc::IOException &e) {
		std::cerr << "Caught IOException(" << e.getMessage() << ")\n";
		return 1;
	}
}
