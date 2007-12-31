/** \file
 * Core module implementation.
 * $Id: application.cpp,v 1.8 2007/12/31 15:39:27 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>

#include "application.hpp"


namespace ppc {


char sharedBuffer[1024];


unsigned long Core::ticks = 0;


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
			++ticks;
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			break;

		default:
			for (Modules::iterator it = modules.begin(), end = modules.end();
			     it!=end && nfds > 0; ++it) {
				nfds -= it->second->doFDs(nfds, &rd, &wr, &ex);
			}
		}

		deliverSignals();
	}

	for (Modules::iterator it = modules.begin(), end = modules.end();
	     it!=end; ++it) {
		delete it->second;
	}

	modules.clear();
	while (!signals.empty()) {
		signals.front().clear();
		signals.pop();
	}
	mainModule = 0;

	return 0;
}


void Core::deliverSignals() {
	while (!signals.empty()) {
		const Signal sig = signals.front();
		signals.front().clear();
		signals.pop();

		const std::string &rec = sig.getReciever();
		const std::string::size_type length = rec.length();
		if (!length) continue;

		Modules::iterator it = modules.lower_bound(rec);
		const Modules::iterator end = modules.end();

		if (it==end) continue;

		if (rec[length - 1] != '/') {
			if (it->second->moduleName == rec) {
				it->second->recievedSignal(sig);
			}
			continue;
		}

		while (it!=end && it->second->moduleName.length() > length &&
		       !memcmp(rec.data(), it->second->moduleName.data(), length)) {
			it->second->recievedSignal(sig);
			++it;
		}
	}
}


bool Core::addModule(Module &module) {
	if (modules.find(module.moduleName) != modules.end()) {
		return false;
	}
	modules.insert(std::make_pair(module.moduleName, &module));
	signals.push(Signal("/core/module/new", moduleName, "/",
	                    new sig::StringData(module.moduleName)));
	return true;
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
	if (sig.getType() == "/core/module/exit") {
		Modules::iterator it = modules.find(sig.getSender());
		if (it == modules.end()) {
			return;
		}

		if (it->second == mainModule) {
			mainModule = 0;
			running = 0;
		}

		modules.erase(it);
		delete it->second;
		signals.push(Signal("/core/module/remove", moduleName, "/",
		                    new sig::StringData(sig.getSender())));

	} else if (sig.getType() == "/net/conn/connect") {
		/* FIXME: TODO */

	}
}


}
