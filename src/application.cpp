/** \file
 * Core module implementation.
 * $Id: application.cpp,v 1.18 2008/01/03 01:58:01 mina86 Exp $
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


/** Array of signal numbers we are going to catch. */
static const int signalNumbers[] = {
#if SIGHUP
	SIGHUP,
#endif
#if SIGINT
	SIGINT,
#endif
#if SIGQUIT
	SIGQUIT,
#endif
#if SIGPIPE
	SIGPIPE,
#endif
#if SIGTERM
	SIGTERM,
#endif
#if SIGUSR1
	SIGUSR1,
#endif
#if SIGUSR2
	SIGUSR2,
#endif
#if SIGCHLD
	SIGCHLD,
#endif
#if SIGWINCH
	SIGWINCH,
#endif
	SIGALRM
};



int Core::run() {
	struct sigaction act, oldact[sizeof signalNumbers/sizeof *signalNumbers];
	sigset_t oldsigset;

	if (modules.size() == 1) {
		return 0;
	}


	sigemptyset(&act.sa_mask);
	for (unsigned i=0; i < sizeof signalNumbers/sizeof *signalNumbers; ++i) {
		sigaddset(&act.sa_mask, signalNumbers[i]);
	}

	sigprocmask(SIG_BLOCK, &act.sa_mask, &oldsigset);
	act.sa_handler = gotsig;
	act.sa_flags = 0;
#if SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#endif
#if SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif

	for (unsigned i=0; i < sizeof signalNumbers/sizeof *signalNumbers; ++i) {
		sigaction(signalNumbers[i], &act, oldact + i);
	}


	if (!ui_modules) {
		killModules("/");
	}
	deliverSignals();

	alarm(1);
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

		nfds = pselect(nfds, &rd, &wr, &ex, 0, &oldsigset);
		if (nfds > 0) {
			for (Modules::iterator it = modules.begin(), end = modules.end();
			     it!=end && nfds > 0; ++it) {
				nfds -= it->second->doFDs(nfds, &rd, &wr, &ex);
			}
		} else if (nfds == 0) {
			fputs("pselect: returned 0\n", stderr);
			break;
		} else if (errno != EINTR) {
			perror("pselect");
			break;
		} else {
			handleUnixSignals();
		}

		deliverSignals();
	}


	for (unsigned i=0; i < sizeof signalNumbers/sizeof *signalNumbers; ++i) {
		sigaction(signalNumbers[i], oldact + i, 0);
	}
	sigprocmask(SIG_SETMASK, &oldsigset, 0);


	if (modules.size() > 1) {
		Modules::iterator it = modules.begin(), end = modules.end();
		for (; it != end; ++it) {
			if (it->first != moduleName) delete it->second;
		}
		modules.clear();
		modules[moduleName] = this;
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
		sendSignal("/core/tick", "/");
		--sigarr[SIGALRM];
	}

	if (sigarr[SIGTERM]) {
		killModules("/");
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
			sendSignal(sigType, "/");
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

		sendSignal("/core/module/new", "/", module.moduleName);
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
			removeModule(nextToKill->moduleName);
		}

	/* Module exits -- remove from list */
	} else if (sig.getType() == "/core/module/exits") {
		removeModule(sig.getSender());

	/* Someone wants someone dead! */
	} else if (sig.getType() == "/core/module/kill") {
		killModules(sig.getData<sig::StringData>()->data);

	/* Start a new module */
	} else if (sig.getType() == "/core/module/start") {
		/* FIXME: TODO */

	}
}



void Core::killModules(const std::string &target) {
	std::pair<Modules::iterator, Modules::iterator> it =
		matchingModules(target);
	if (it.first == it.second) return;

	const unsigned long dueTime = Core::getTicks() + 60;

	sendSignal("/core/module/quit", target);

	for (; it.first != it.second; ++it.first) {
		if (it.first->second->prevToKill) continue;

		Module &module = *it.first->second;
		module.dieDueTime = dueTime;
		module.prevToKill = prevToKill;
		module.nextToKill = this;
		prevToKill->nextToKill = &module;
		prevToKill = &module;
	}
}



void Core::removeModule(const std::string &name) {
	Modules::iterator it = modules.find(name);
	if (it == modules.end()) {
		return;
	}

	if (it->second->prevToKill) {
		it->second->prevToKill->nextToKill = it->second->nextToKill;
		it->second->nextToKill->prevToKill = it->second->prevToKill;
	}

	delete it->second;
	modules.erase(it);
	sendSignal("/core/module/removed", "/", name);

	if (name.length()>4 && !memcmp(name.data(), "/ui/", 4) && !--ui_modules) {
		killModules("/");
	}
}



}
