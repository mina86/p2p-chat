/** \file
 * A netio library UDP comunication test.
 * Copyright 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include "../netio.hpp"


int main(int argc, char **argv) {
	bool server = argc <= 1;
	ppc::Address addr(server ? ppc::IP() : ppc::IP(argv[1]), 8888);
	ppc::UDPSocket sock(server ? addr : ppc::Address());
	int ret;

	ret = 0;
	for(;;) {
		int num = 0;
		fd_set rd, wr;

		FD_ZERO(&rd);
		FD_ZERO(&wr);
		FD_SET(0, &rd);
		if (server) {
			FD_SET(num = sock.fd, &rd);
		} else if (sock.hasDataToWrite()) {
			FD_SET(num = sock.fd, &wr);
		}

		num = select(num + 1, &rd, &wr, 0, 0);
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

			if (!server) {
				sock.push(ppc::sharedBuffer, addr);
			}
		}

		if (FD_ISSET(sock.fd, &rd)) {
			std::string data;
			while (!(data = sock.read(addr)).empty()) {
				printf("[%s]: %s", addr.toString().c_str(), data.c_str());
			}
		}

		if (FD_ISSET(sock.fd, &wr)) {
			sock.write();
		}
	}

	return ret;
}
