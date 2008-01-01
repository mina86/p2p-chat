/** \file
 * Core module implementation.
 * $Id: application.cpp,v 1.13 2008/01/01 19:30:35 mina86 Exp $
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
	} else if (!ui_modules) {
		sendSignal("/core/module/kill", moduleName, 0);
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
			sendSignal("/core/tick", "/", 0);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
		} else if (errno != EINTR) {
			perror("select");
			break;
		} else {
			sendSignal("/ui/msg/notice", "/ui/",
			           new sig::StringData("Recieved signal, terminating."));
			sendSignal("/core/module/kill", moduleName, 0);
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
	}

	if (reciever[length - 1] != '/') {
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
