/** \file
 * Core module implementation.
 * $Id: application.cpp,v 1.12 2008/01/01 03:22:55 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <limits>

#include "application.hpp"


namespace ppc {


char sharedBuffer[1024];

unsigned long Core::ticks = 0;

/**
 * Signal handler which does nothing.
 * \param signum signal number.
 */
static void gotsig(int signum) {
	signal(signum, gotsig);
}



int Core::run() {
	struct timeval tv = { 1, 0 };

	if (modules.size() == 1) {
		return 0;
	} else if (ui_modules) {
		dieDueTime = std::numeric_limits<unsigned long>::max();
	} else {
		dieDueTime = Core::getTicks() + 60;
		sendSignal("/core/module/quit", "/", 0);
	}


#if SIGHUP
	signal(SIGHUP , gotsig);
#endif
#if SIGINT
	signal(SIGINT , gotsig);
#endif
#if SIGQUIT
	signal(SIGQUIT, gotsig);
#endif
#if SIGTERM
	signal(SIGTERM, gotsig);
#endif


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
		if (nfds > 0) {
			for (Modules::iterator it = modules.begin(), end = modules.end();
			     it!=end && nfds > 0; ++it) {
				nfds -= it->second->doFDs(nfds, &rd, &wr, &ex);
			}
		} else if (nfds == 0) {
			if (++ticks >= dieDueTime) break;
			sendSignal("/core/tick", "/", 0);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
		} else if (errno != EINTR) {
			perror("select");
			break;
		} else {
			unsigned long newDueTime = Core::getTicks() + 60;
			dieDueTime = newDueTime > dieDueTime ? newDueTime : dieDueTime;
			sendSignal("/ui/msg/notice", "/ui/",
			           new sig::StringData("Recieved signal, terminating."));
			sendSignal("/core/module/quit", "/", 0);
		}

		deliverSignals();
	}


#if SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
#if SIGINT
	signal(SIGINT, SIG_DFL);
#endif
#if SIGQUIT
	signal(SIGQUIT, SIG_DFL);
#endif
#if SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif


	if (modules.size() > 1) {
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
	for (; !signals.empty(); signals.front().clear(), signals.pop()) {
		const std::string &rec = signals.fron().getReciever();
		const std::string::size_type length = rec.length();
		if (!length) continue;

		Modules::iterator it = modules.lower_bound(rec);
		const Modules::iterator end = modules.end();
		if (it==end) continue;

		if (rec[length - 1] != '/') {
			if (it->second->moduleName == rec) {
				it->second->recievedSignal(signals.front());
			}
			continue;
		}

		while (it!=end && it->second->moduleName.length() > length &&
		       !memcmp(rec.data(), it->second->moduleName.data(), length)) {
			it->second->recievedSignal(signals.front());
			++it;
		}
	}
}


bool Core::addModule(Module &module) {
	std::pair<Modules::iterator, bool> ret =
		modules.insert(std::make_pair(module.moduleName, &module));
	if (ret.second) {
		ui_modules += module.moduleName.length() > 4 &&
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
	if (sig.getType() == "/core/module/exits") {
		Modules::iterator it = modules.find(sig.getSender());
		if (it == modules.end()) {
			return;
		}

		modules.erase(it);
		delete it->second;
		sendSignal("/core/module/removed", "/",
		           new sig::StringData(sig.getSender()));

		ui_modules -= sig.getSender().length() >= 4 &&
			!memcmp(sig.getSender().data(), "/ui/", 4);
		if (!ui_modules) {
			sendSignal("/core/module/quit", "/", 0);
			dieDueTime = Core::getTicks() + 60;
		}

	} else if (sig.getType() == "/core/module/start") {
		/* FIXME: TODO */

	}
}


}
