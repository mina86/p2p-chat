/** \file
 * Main file.
 * $Id: main.cpp,v 1.3 2008/01/12 02:26:38 mina86 Exp $
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "application.hpp"
#include "network.hpp"
#include "ui.hpp"


/** Main function. */
int main(int argc, char **argv) {

	if (argc != 4) {
		fprintf(stderr, "usage: %s <ip-address> <port> <nick>\n", *argv);
		return 1;
	}

	ppc::Address address(ppc::IP(argv[1]), 0);
	if (!address.ip.isMulticast()) {
		fprintf(stderr, "%s: %s: invalid multicast IP address\n",
		        argv[0], argv[1]);
		return 1;
	}

	{
		char *end;
		long val;
		errno = 0;
		val = strtol(argv[2], &end, 10);
		if (errno || val <= 1024 || val > 65536 || *end) {
			fprintf(stderr, "%s: %s: invalid port number\n",
			        argv[0], argv[2]);
			return 1;
		}
		address.port = val;
	}

	std::string nick(argv[3]);
	if (!ppc::User::isValidNick(nick)) {
		fprintf(stderr, "%s: %s: invalid nick name\n", argv[0], argv[3]);
			return 1;
	}

	struct ppc::Config c;
	struct ppc::Core core(c);
	core.addModule(*new ppc::Network(core, address, nick));
	core.addModule(*new ppc::UI(core));
	return core.run();
}
