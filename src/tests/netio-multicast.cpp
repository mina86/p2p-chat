/** \file
 * A netio library UDP multicast comunication test.
 * $Id: netio-multicast.cpp,v 1.1 2008/01/06 15:26:53 mina86 Exp $
 */

#include <stdio.h>

#include "../netio.hpp"

int run(ppc::IP ip) {
	ppc::Address addr(ip, 8888);
	ppc::UDPSocket sock(addr);
	int ret;

	ret = 0;
	for(;;) {
		int num;
		fd_set rd, wr;

		FD_ZERO(&rd);
		FD_ZERO(&wr);
		FD_SET(0, &rd);
		FD_SET(sock.fd, &rd);
		if (sock.hasDataToWrite()) {
			FD_SET(sock.fd, &wr);
		}

		num = select(sock.fd + 1, &rd, &wr, 0, 0);
		if (num < 0) {
			perror("select");
			break;
		} else if (num == 0) {
			fputs("select: returned 0\n", stderr);
			break;
		}

		if (FD_ISSET(0, &rd)) {
			if (!fgets(ppc::sharedBuffer, sizeof ppc::sharedBuffer, stdin)) {
				break;
			}
			sock.push(ppc::sharedBuffer, addr);
		}

		if (FD_ISSET(sock.fd, &rd)) {
			ppc::Address remoteAddr;
			std::string data;
			while (!(data = sock.read(remoteAddr)).empty()) {
				printf("[%s]: %s",
				       remoteAddr.toString().c_str(), data.c_str());
			}
		}

		if (FD_ISSET(sock.fd, &wr)) {
			sock.write();
		}
	}

	return ret;
}


int main(int argc, char **argv) {
	try {
		return run(argc<2 ? ppc::IP(0xe1424242) : ppc::IP(argv[1]));
	}
	catch (const ppc::IOException &e) {
		fprintf(stderr, "IOException: %s\n", e.getMessage().c_str());
		return 1;
	}
}
