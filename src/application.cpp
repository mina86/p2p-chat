/** \file
 * Core module implementation.
 * $Id: application.cpp,v 1.14 2008/01/01 21:29:20 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "application.hpp"


namespace ppc {


char sharedBuffer[1024];

unsigned long Core::ticks = 0;

/** Stores number of times each unix signal was delivered. */
static volatile sig_atomic_t sigarr[32];


/**
 * Signal handler which counts signals.
 * \param signum signal number.
 */
static void gotsig(int signum) {
	++sigarr[0];
	++sigarr[signum];
}



int Core::run() {
	struct sigaction act, oldact[32];
	sigset_t oldsigset;

	if (modules.size() == 1) {
		return 0;
	} else if (!ui_modules) {
		sendSignal("/core/module/kill", moduleName, 0);
	}


	sigfillset(&act.sa_mask);
	sigdelset(&act.sa_mask, SIGKILL);
	sigdelset(&act.sa_mask, SIGSTOP);

	sigprocmask(SIG_BLOCK, &act.sa_mask, &oldsigset);
	act.sa_handler = gotsig;
	act.sa_flags = 0;
#if SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#endif
#if SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif

	for (int i = 1; i < 32; ++i) {
		sigaction(SIGHUP , &act, oldact + SIGHUP);
	}


	alarm(1);
	while (modules.size() > 1) {
		fd_set rd, wr, ex;
		int nfds = 0;

		deliverSignals();

		FD_ZERO(&rd);
		FD_ZERO(&wr);
		FD_ZERO(&ex);

		for (Modules::iterator it = modules.begin(), end = modules.end();
		     it!=end; ++it) {
			int n = it->second->setFDSets(&rd, &wr, &ex);
			if (n > nfds) nfds = n;
		}

		nfds = pselect(nfds, &rd, &wr, &ex, 0, &oldsigset);
		if (nfds > 0) {
			for (Modules::iterator it = modules.begin(), end = modules.end();
			     it!=end && nfds > 0; ++it) {
				nfds -= it->second->doFDs(nfds, &rd, &wr, &ex);
			}
		} else if (nfds == 0) {
			fputs("select: returned 0\n", stderr);
			break;
		} else if (errno != EINTR) {
			perror("select");
			break;
		} else {
			handleUnixSignals();
		}
	}

	for (int i = 1; i < 32; ++i) {
		sigaction(i, oldact + i, 0);
	}
	sigprocmask(SIG_SETMASK, &oldsigset, 0);

	if (modules.size() > 1) {
		Modules::iterator it = modules.begin(), end = modules.end();
		for (; it != end; ++it) {
			if (it->first != moduleName) delete it->second;
		}
		modules.clear();
		modules.insert(std::make_pair(moduleName,static_cast<Module*>(this)));
		prevToKill = nextToKill = this;
	}

	while (!signals.empty()) {
		signals.front().clear();
		signals.pop();
	}

	return 0;
}


void Core::deliverSignals() {
	for (; !signals.empty(); signals.front().clear(), signals.pop()) {
		std::pair<Modules::iterator, Modules::iterator> it
			= matchingModules(signals.front().getReciever());
		for (; it.first != it.second; ++it.first) {
			it.first->second->recievedSignal(signals.front());
		}
	}
}


std::pair<Module::Modules::iterator, Module::Modules::iterator>
Core::matchingModules(const std::string &reciever) {
	const std::string::size_type length = reciever.length();
	const Modules::iterator end = modules.end();

	if (!length || reciever[0] != '/') {
		return std::make_pair(end, end);
	} else if (length == 1) {
		return std::make_pair(modules.begin(), end);
	} else if (reciever[length - 1] != '/') {
		Modules::iterator it = modules.find(reciever), last = it;
		if (it == end) {
			return std::make_pair(end, end);
		} else {
			return std::make_pair(it, ++last);
		}
	} else {
		Modules::iterator first = modules.lower_bound(reciever), last = first;
		while (last!=end && last->second->moduleName.length() > length &&
		       !memcmp(reciever.data(), last->second->moduleName.data(),
		               length)) {
			++last;
		}
		return std::make_pair(first, last);
	}
}



void Core::handleUnixSignals() {
	int i = 1;

	sigarr[0] -= sigarr[SIGALRM];
	while (sigarr[SIGALRM] > 0) {
		sendSignal("/core/tick", "/", 0);
		--sigarr[SIGALRM];
	}

	while (sigarr[0] > 0) {
		while (sigarr[i] == 0 && i < 32) ++i;
		if (i == 32) {
			break;
		}

		int j = sigarr[i];
		sigarr[0] -= j;
		sigarr[i] = 0;
		sprintf(sharedBuffer, "/core/sig/%d", i);
		std::string sigType(sharedBuffer);
		do {
			sendSignal(sigType, "/", 0);
		} while (--j);
	}

	sigarr[0] = 0; /* just to be on the safe side */
}



bool Core::addModule(Module &module) {
	if (module.moduleName.empty() || module.moduleName[0] != '/' ||
	    module.moduleName[module.moduleName.length() - 1] == '/') {
		return false;
	}

	std::pair<Modules::iterator, bool> ret =
		modules.insert(std::make_pair(module.moduleName, &module));
	if (ret.second) {
		ui_modules += module.moduleName.length() > 4 &&
			!memcmp(module.moduleName.data(), "/ui/", 4);

		module.dieDueTime = std::numeric_limits<unsigned long>::max();
		module.prevToKill = module.nextToKill = 0;

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
	/* Some modules are due to die? */
	if (sig.getType() == "/core/tick") {
		while (nextToKill->dieDueTime <= Core::getTicks()) {
			Module *m = nextToKill;

			nextToKill->nextToKill->prevToKill = this;
			nextToKill = nextToKill->nextToKill;

			modules.erase(modules.find(m->moduleName));
			sendSignal("/core/module/removed", "/",
			           new sig::StringData(m->moduleName));
			delete m;
		}


	/* Module exits -- remove from list */
	} else if (sig.getType() == "/core/module/exits") {
		Modules::iterator it = modules.find(sig.getSender());
		if (it == modules.end()) {
			return;
		}

		if (it->second->prevToKill) {
			it->second->prevToKill->nextToKill = it->second->nextToKill;
			it->second->nextToKill->prevToKill = it->second->prevToKill;
		}

		delete it->second;
		modules.erase(it);
		sendSignal("/core/module/removed", "/",
		           new sig::StringData(sig.getSender()));

		ui_modules -= sig.getSender().length() >= 4 &&
			!memcmp(sig.getSender().data(), "/ui/", 4);
		if (!ui_modules) {
			sendSignal("/core/module/quit", "/", 0);
			dieDueTime = Core::getTicks() + 60;
		}


	/* Someone wants someone dead! */
	} else if (sig.getType() == "/core/module/kill") {
		const std::string &target = sig.getData()
			? static_cast<const sig::StringData*>(sig.getData())->data : "/";
		std::pair<Modules::iterator, Modules::iterator> it =
			matchingModules(target);
		unsigned long dueTime = Core::getTicks() + 60;

		sendSignal("/core/module/quit", target, 0);

		for (; it.first != it.second; ++it.first) {
			if (it.first->second->prevToKill) continue;

			Module &module = *it.first->second;
			module.dieDueTime = dueTime;
			module.prevToKill = prevToKill;
			module.nextToKill = this;
			prevToKill->nextToKill = &module;
			prevToKill = &module;
		}


	/* Start a new module */
	} else if (sig.getType() == "/core/module/start") {
		/* FIXME: TODO */

	}
}


}
