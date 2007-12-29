/** \file
 * User interface implementation.
 * $Id: ui.cpp,v 1.3 2007/12/29 14:40:40 mina86 Exp $
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "ui.hpp"


namespace ppc {


/**
 * A private (file-scope) variable to make sequential numbers in
 * module names.  Each file implementing each module (should) have its
 * own \a seq variable.
 */
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

	/* read the data from stdin_fd and put it inside data.  If user
	   typed enter you do the rest of the method otherwise you return
	   from it.  */
	std::string data = "command user typed in";
	handleCommand(data);

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


void UI::handleCommand(const std::string &command) {
	std::pair<std::string::size_type, std::string::size_type> pos
		= nextToken(command);
	std::string::size_type len = pos.second - pos.first;

	if (!len) {
		return;
	}

	std::string data(command, pos.first, len);
	if (command[pos.first] != '/') {
		pos.second = pos.first;
		goto message;
	}

	if (len == 1 || (len == 4 && data == "/msg") ||
	    (len == 3 && data == "/me")) {
		/* if len == 0 then data == "/" */
	message:
		/* trim */
		pos = nextToken(command, pos.second);
		if (pos.first == std::string::npos) {
			/* refuse to send empty line */
			return;
		}

		/* somhow identify network and user you want to send data to. */
		std::string net = "/net/ppcp/0";
		User *user = 0;
		sendSignal("/net/msg/send", net,
		           new sig::MessageData(user->id,
		                                std::string(command, pos.first),
		                                len==3?sig::MessageData::ACTION:0));

	} else if (len == 3 && data == "/aw") {
		/* dirty trick */
		pos.second = pos.first + 1;
		goto status;

	} else if (len == 7 && data == "/status") {
	status:
		enum User::State state;

		pos = nextToken(command, pos.second);
		if (pos.first == std::string::npos) {
			return;
		}

		data.assign(command, pos.first, pos.second - pos.first);
		if (data == "online" || data == "on") {
			state = User::ONLINE;
		} else if (data == "away" || data == "aw") {
			state = User::AWAY;
		/* ... */
		} else {
			/* print error message -- unkonw state */
			return;
		}

		/* somhow identify network */
		std::string net = "/net/ppcp/0";
		pos = nextToken(command, pos.second);
		if (pos.first == std::string::npos) {
			data.assign(command, pos.first, std::string::npos);
		} else {
			data.clear();
		}

		sendSignal("/user/status/change", net,
		           new sig::UserData(User("dummy", Address(),
		                                  User::Status(state, data)),
		                             sig::UserData::STATE |
		                             sig::UserData::MESSAGE));

	/* ... and so it may go with other commands */
	} else {
		/* print error message -- unkonw command */
	}
}



std::pair<std::string::size_type, std::string::size_type>
UI::nextToken(const std::string &str, std::string::size_type pos) {
	std::string::size_type start, end = std::string::npos;

	/* you could do better here and interprete ' and " */

	start = str.find_first_not_of(" \t\n\v\r", pos);
	if (start != std::string::npos) {
		end = str.find_first_of(" \t\n\v\r", start + 1);
		if (end == std::string::npos) {
			end = str.length();
		}
	}

	return std::make_pair(start, end);
}



}
