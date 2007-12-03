/**
 * Core module implementation.
 * $Id: application.cpp,v 1.1 2007/12/03 14:46:33 mina86 Exp $
 */

#include "application.hpp"

namespace ppc {


int Core::run() {
	/* TODO */
	return 0;
}


int Core::setFDSets(fd_set *rd, fd_set *wr, fd_set *ex) {
	(void)rd; (void)wr; (void)ex;
	return 0;
}

int Core::doFDs(const fd_set *rd, const fd_set *wr, const fd_set *ex) {
	(void)rd; (void)wr; (void)ex;
	return 0;
}


void Core::recievedSignal(const Signal &sig) {
	(void)sig;
	/* TODO */
}

}
