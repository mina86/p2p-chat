/** \file
 * Main file.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "application.hpp"
#include "config.hpp"
#include "network.hpp"
#include "ui.hpp"
#include "sounds.hpp"


/**
 * Main function.
 * \param argc number of arguments.
 * \param argv arguments array.
 */
int main(int argc, char **argv) {
	int ret;

	if (argc != 4) {
		fprintf(stderr, "usage: %s <ip-address> <port> <nick>\n", *argv);
		return 1;
	}

	ppc::Address address = ppc::Address(ppc::IP(argv[1]), 0);
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

	struct ppc::ConfigFile config("ppcrc");;
	struct ppc::Core core(config);

	core.addModule(*new ppc::Network(core, address, nick));
	core.addModule(*new ppc::UI(core));
	core.addModule(*new ppc::SoundsUI(core));
	ret = core.run();

	config.saveConfig();
	return ret;
}
