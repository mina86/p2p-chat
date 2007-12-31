/** \file
 * Core module implementation.
 * $Id: application.cpp,v 1.9 2007/12/31 19:35:41 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>

#include <limits>

#include "application.hpp"


namespace ppc {


char sharedBuffer[1024];


unsigned long Core::ticks = 0;


int Core::run() {
	struct timeval tv = { 1, 0 };

	if (ui_modules) {
		dieDueTime = std::numeric_limits<unsigned long>::max();
	} else {
		dieDueTime = Core::getTicks() + 60;
		sendSignal("/core/module/quit", "/", 0);
	}

	while (modules.size() > 1) {
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
			if (++ticks >= dieDueTime) goto killAllModules;

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

		deliverSignals();
	}

	if (modules.size() > 1) {
	killAllModules:
		Modules::iterator it = modules.begin(), end = modules.end();
		for (; it != end; ++it) {
			if (it->first != moduleName) delete it->second;
		}
		modules.clear();
		modules.insert(std::make_pair(moduleName,static_cast<Module*>(this)));
	}

	while (!signals.empty()) {
		signals.front().clear();
		signals.pop();
	}

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
	std::pair<Modules::iterator, bool> ret =
		modules.insert(std::make_pair(module.moduleName, &module));
	if (ret.second) {
		ui_modules += module.moduleName.length() >= 4 &&
			!memcmp(module.moduleName.data(), "/ui/", 4);
		sendSignal("/core/module/new", "/",
		           new sig::StringData(module.moduleName));
	}
	return ret.second;
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

		modules.erase(it);
		delete it->second;
		sendSignal("/core/module/remove", "/",
		           new sig::StringData(sig.getSender()));

		ui_modules -= sig.getSender().length() >= 4 &&
			!memcmp(sig.getSender().data(), "/ui/", 4);
		if (!ui_modules) {
			sendSignal("/core/module/quit", "/", 0);
			dieDueTime = Core::getTicks() + 60;
		}

	} else if (sig.getType() == "/net/conn/connect") {
		/* FIXME: TODO */

	}
}


}
