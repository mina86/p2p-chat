/** \file
 * Core module implementation.
 * $Id: application.cpp,v 1.2 2007/12/03 23:49:19 mina86 Exp $
 */

#include <stdio.h>

#include "application.hpp"

namespace ppc {


int Core::run() {
	struct timeval tv = { 1, 0 };

	while (running) {
		fd_set rd, wr, ex;
		int nfds = 0;

		FD_ZERO(&rd);
		FD_ZERO(&wr);
		FD_ZERO(&ex);

		for (Modules::iterator it = modules.begin(), end = modules.end();
		     it!=end; ++it) {
			int n = it->second->setFDSets(&rd, &wr, &ex);
			if (n > nfds) nfds = n;
		}

		nfds = select(nfds, &rd, &wr, &ex, &tv);
		switch (nfds) {
		case -1:
			perror("select");
			return 1;

		case  0:
			sendSignal("/core/tick", "/", 0);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			break;

		default:
			for (Modules::iterator it = modules.begin(), end = modules.end();
			     it!=end && nfds > 0; ++it) {
				nfds -= it->second->doFDs(nfds, &rd, &wr, &ex);
			}
		}

		while (!signals.empty()) {
			const Signal sig = signals.front();
			const std::string &rec = sig.getReciever();
			signals.pop();
			if (rec.empty()) continue;

			Modules::iterator it, end = modules.end();
			/* TODO */

		}
	}
	return 0;
}


int Core::setFDSets(fd_set *rd, fd_set *wr, fd_set *ex) {
	(void)rd; (void)wr; (void)ex;
	return 0;
}

int Core::doFDs(int nfds, const fd_set *rd, const fd_set *wr,
                const fd_set *ex) {
	(void)nfds; (void)rd; (void)wr; (void)ex;
	return 0;
}


void Core::recievedSignal(const Signal &sig) {
	(void)sig;
	/* TODO */
}

}
