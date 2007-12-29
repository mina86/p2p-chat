/** \file
 * User interface implementation.
 * $Id: ui.cpp,v 1.1 2007/12/29 02:39:04 mina86 Exp $
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "ui.hpp"


namespace ppc {


static unsigned long seq = 0;


UI::UI(Core &c, int infd /* some more arguments */)
	: Module(c, "/ui/mco/", seq++), stdin_fd(infd) {

	int flags = fcntl(infd, F_GETFL);
	if (flags < 0 || fcntl(infd, F_SETFL, flags | O_NONBLOCK) < 0) {
		/* maybe another exception class */
		throw Exception(std::string("fcntl: ") + strerror(errno));
	}

	/* disable all kinds of buffering */

	/* ask network modules if they are connected, we'll get whole
	   a lot of /net/conn/connected signals if they are connected
	   signals */
	sendSignal("/net/conn/are-you-connected", "/net/", 0);
}


UI::~UI() {
	/* whatever needed */
}


int UI::setFDSets(fd_set *rd, fd_set *wr, fd_set *ex) {
	(void)wr; (void)ex;
	FD_SET(stdin_fd, rd);
	return stdin_fd;
}


int UI::doFDs(int nfds, const fd_set *rd, const fd_set *wr,
                  const fd_set *ex) {
	(void)nfds; (void)wr; (void)ex;

	if (!FD_ISSET(stdin_fd, rd)) {
		return 0;
	}

	/* read the data from stdin_fd */

	return 1;
}


void UI::recievedSignal(const Signal &sig) {
	if (sig.getType() == "/core/tick") {
		/* one second have passed, maybe you need to do something? */


	} else if (sig.getType() == "/core/module/new") {
		/* a new module added to list, probably nothing to do here but
		   who knows */
		std::string module_name =
			static_cast<const sig::StringData*>(sig.getData())->data;
		(void)module_name; /* to silence warning for the time being */

	} else if (sig.getType() == "/core/module/remove") {
		/* module have been removed, here you probably need to do
		   something if you are the main module like exit if user
		   requested us to quit */
		std::string module_name =
			static_cast<const sig::StringData*>(sig.getData())->data;
		(void)module_name; /* to silence warning for the time being */

		if (/*exiting && */ core.getMainModule() == this &&
		    getModules().size() == 2 /* there is only us and core */) {
			/* just send signal to core that we wish to exit, core
			   will know what to do -- since we are main module it
			   will destroy everything (the entire planet except for
			   IBM which is unbrekable) */
			sendSignal("/core/module/exit", "/core", 0);
		}


	} else if (sig.getType() == "/net/status/changed") {
		/* a user have changed status or display name or have just
		   connected ot it have jsut disconnected, it's all in data.
		   You may (shoul) identify network which sent information by
		   sig.getSender(). */
		const sig::UserData &data =
			*static_cast<const sig::UserData*>(sig.getData());
		(void)data; /* to silence warning for the time being */


	} else if (sig.getType() == "/net/users/rp") {


	} else if (sig.getType() == "/net/msg/got") {
		/* we have recieved a message (display it maybe?) */
		const sig::MessageData &data =
			*static_cast<const sig::MessageData*>(sig.getData());
		(void)data; /* to silence warning for the time being */

	} else if (sig.getType() == "/net/msg/sent") {
		/* a message have been sent (display it maybe?) */
		const sig::MessageData &data =
			*static_cast<const sig::MessageData*>(sig.getData());
		(void)data; /* to silence warning for the time being */

	} else if (sig.getType() == "/net/conn/connected") {
		/* wow! a new network :) and it sents us its user list.
		   Signal's argument is a sig::UsersListData which you may
		   refer to while the network is connected.  Yes! you don't
		   need to resent /net/users/rq signal each time.  Object
		   pointed to by sig.getData() is the same object that network
		   uses to store information about users so you may use it to
		   find users etc -- no need to mainain your own database.
		   But BEWARE!!!!  YOU MUST STORE A shared_obj OBJECT AND NOT
		   POINTER ITSELF otherwise data will be deleted when network
		   object is deleted. */

		const sig::UsersListData *data =
			static_cast<const sig::UsersListData*>(sig.getData());

		/* sorry -- couldn't think of better way, the const_cast
		   is required */
		networkUsers[sig.getSender()] = const_cast<sig::UsersListData*>(data);

	} else if (sig.getType() == "/net/conn/disconnected") {
		/* ok, so we have disconnected -- time to clear all resources
		   associated with given network; also informing user is not
		   that bad idea.  Cleaning reasources may be done like this: */

		networkUsers.erase(networkUsers.find(sig.getSender()));

	} else if (!strncmp(sig.getType().c_str(), "/ui/msg/", 8)) {
		/* this is some kind of message.  Type is one of:
		   /ui/msg/debug, /ui/msg/info, /ui/msg/notice or
		   /ui/msg/error.  You may choose to display that message
		   (especially if it's an error */
		std::string message =
			static_cast<const sig::StringData*>(sig.getData())->data;
		(void)message; /* to silence warning for the time being */

	}
}


}
