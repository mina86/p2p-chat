/** \file
 * User interface implementation.
 * $Id: ui.cpp,v 1.26 2008/01/21 20:03:54 mco Exp $
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "io.hpp"
#include "ui.hpp"

/** Output window buffer size. */
#define PPC_UI_OUTPUTWINDOW_BUFFERSIZE	4096
/** Number of entries to display when user name is ambiguous. */
#define PPC_UI_COMPLETIONMENUSIZE 10
/** Command history size. */
#define PPC_UI_HISTORY_SIZE		100


namespace ppc {


/**
 * A private (file-scope) variable to make sequential numbers in
 * module names.  Each file implementing each module (should) have its
 * own \a seq variable.
 */
static unsigned long seq = 0;


UI::UI(Core &c, int infd /* some more arguments */)
	: Module(c, "/ui/mco/", seq++), stdin_fd(infd),
	chatUser(std::string(), Address()) {

	FileDescriptor::setNonBlocking(infd);

	/* enable windowed mode */
	initscr();

	/* disable all kinds of buffering */
	cbreak();
	noecho();

	/* create windows */
	getmaxyx(stdscr, maxY, maxX);
	messageW = new OutputWindow( this, maxY-2, maxX,       0, 0);
	statusW =                  newwin(      1, maxX, maxY- 2, 0);
	commandW = new CommandWindow(this,      1, maxX, maxY- 1, 0);

	keypad(stdscr, true);
	nodelay(stdscr, true);

	/* initialize history buffers */
	history.push_front(std::string(""));
	historyIterator = history.begin();
	commandCurPos = 0;

	/* ask network modules if they are connected, we'll get whole
	   a lot of /net/conn/connected signals if they are connected
	   signals */
	sendSignal("/net/conn/are-you-connected", "/net/");
	messageW->printf("Hello from User Interface on fd=%d\n", infd);
	messageW->printf("Enter: %d, %d, %d\n", '\n', '\r', KEY_ENTER);
	messageW->printf("backspace, delchar, erasechar: %d, %d, %d, \n",
	                 KEY_BACKSPACE, KEY_DC, erasechar());
	messageW->printf("UP, DOWN, LEFT, RIGHT: %d %d %d %d\n",
	                 KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT);
	wprintw(statusW, "W trakcie tworzenia");
	wnoutrefresh(stdscr);
	messageW->refresh();
	wnoutrefresh(statusW);
	commandW->refresh();
	doupdate();
}


UI::~UI() {
	/* destroy windows */
	delete messageW;
	delwin(statusW);
	delete commandW;

	/* whatever needed */
	endwin();
}


int UI::setFDSets(fd_set *rd, fd_set *wr, fd_set *ex) {
	(void)wr; (void)ex;
	FD_SET(stdin_fd, rd);
	return stdin_fd+1;
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
	/* std::string data = "command user typed in"; */
	/*	std::string data;
	scanf("%s", data.c_str());
	*/

	int c;
	if((c = getch()) == ERR) {
		return -1;
	}

	handleCharacter(c);
	return 1;
}


void UI::recievedSignal(const Signal &sig) {
	if (sig.getType() == "/core/tick") {
		/* one second have passed, maybe you need to do something? */


	} else if (sig.getType() == "/net/status/changed") {
		/* a user have changed status or display name or have just
		   connected ot it have jsut disconnected, it's all in data.
		   You may (shoul) identify network which sent information by
		   sig.getSender(). */
		handleSigStatusChanged(sig.getSender(),
		                       *sig.getData<sig::UserData>());
		messageW->refresh(true);


	} else if (sig.getType() == "/net/msg/got") {
		const sig::MessageData &data = *sig.getData<sig::MessageData>();
		messageW->printf(data.flags & sig::MessageData::ACTION
		                 ? " * %s %s\n" : " <%s> %s\n",
		                 userName(sig.getSender(), data.id).c_str(),
		                 data.data.c_str());
		messageW->refresh(true);

	} else if (sig.getType() == "/net/msg/sent") {
		const sig::MessageData &data = *sig.getData<sig::MessageData>();
		if (data.flags & sig::MessageData::RAW) {
			/* nothing */
		} else if (~data.flags & sig::MessageData::ACTION) {
			messageW->printf("> %s\n", data.data.c_str());
		} else {
			messageW->printf(" * %s %s\n",
			                 ourUserName(sig.getSender()).c_str(),
			                 data.data.c_str());
		}
		messageW->refresh(true);


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

		const sig::UsersListData *data = sig.getData<sig::UsersListData>();

		/* sorry -- couldn't think of better way, the const_cast
		   is required */
		networkUsers[sig.getSender()] = const_cast<sig::UsersListData*>(data);

	} else if (sig.getType() == "/core/module/removed") {
		/* module have been removed; it might be a network */
		std::string module_name = sig.getData<sig::StringData>()->data;
		networkUsers.erase(networkUsers.find(module_name));

	} else if (!strncmp(sig.getType().c_str(), "/ui/msg/", 8)) {
		/* this is some kind of message.  Type is one of:
		   /ui/msg/debug, /ui/msg/info, /ui/msg/notice or
		   /ui/msg/error.  You may choose to display that message
		   (especially if it's an error */
		messageW->printf("[%s] %s\n", sig.getType().c_str() + 8,
		        sig.getData<sig::StringData>()->data.c_str());
		messageW->refresh();
		commandW->redraw();


	} else if (sig.getType() == "/core/module/quit") {
		sendSignal("/core/module/exits", Core::coreName);

	}
}

void UI::handleCharacter(int c) {
	std::list<std::string>::iterator tmpHistoryIterator;

	mvwprintw(statusW, 0, 0, "Entered character: [O:%o] [D:%d] [H:%x]", c, c, c);
	wclrtoeol(statusW);
	wnoutrefresh(statusW);
	doupdate();

	switch(c) {
		/* tab key */
	case '\t':
		mvwprintw(statusW, 0, 0, "TAB-completion not supported, donations welcome");
		wclrtoeol(statusW);
		wnoutrefresh(statusW);
		doupdate();
		break;
		/* enter key */
	case '\n':
	case '\r':
	case KEY_ENTER:
		if(historyIterator->length() == 0) {
			break;
		}
		if(historyIterator != history.begin()) {
			*history.begin() = *historyIterator;
		}
		handleCommand(*history.begin());
		messageW->refresh();
		if(history.size() > 1
		&& *historyIterator == *(++history.begin())) {
			history.begin()->clear();
		} else {
			history.push_front(std::string(""));
		}
		historyIterator = history.begin();
		commandCurPos = 0;
		if(history.size() > PPC_UI_HISTORY_SIZE) {
			history.pop_back();
		}
		break;

		/* backspace key */
	case 0x07:
	case 0x08:
	case KEY_BACKSPACE:
		if(historyIterator != history.begin()) {
			*history.begin() = *historyIterator;
			historyIterator = history.begin();
		}
		if(commandCurPos > 0 && commandCurPos <= historyIterator->length()) {
			historyIterator->erase(commandCurPos-1, 1);
			--commandCurPos;
		}
		break;

		/* delete key */
	case 0x04:
	case KEY_DC:
		if(commandCurPos < historyIterator->length()) {
			historyIterator->erase(commandCurPos, 1);
		}
		break;

	case 0x10:
	case KEY_UP:
		tmpHistoryIterator = historyIterator;
		++tmpHistoryIterator;
		if(tmpHistoryIterator != history.end()) {
			historyIterator=tmpHistoryIterator;
			commandCurPos = historyIterator->length();
		}
		break;

	case 0x0e:
	case KEY_DOWN:
		if(historyIterator != history.begin()) {
			--historyIterator;
			commandCurPos = historyIterator->length();
		}
		break;

	case 0x02:
	case KEY_LEFT:
		if(commandCurPos>0) {
			--commandCurPos;
		}
		break;

	case 0x06:
	case KEY_RIGHT:
		if(commandCurPos < historyIterator->size()) {
			++commandCurPos;
		}
		break;

	case 0x01:
	case KEY_HOME:
		commandCurPos = 0;
		break;

	case 0x05:
	case KEY_END:
		commandCurPos = historyIterator->size();
		break;

	default:
		if(commandCurPos == maxX) {
			mvwprintw(statusW, 0, 0, "No ziomal, krutsze som te polecenia Yo");
			wclrtoeol(statusW);
			wnoutrefresh(statusW);
			doupdate();
			break;
		}
		if(historyIterator != history.begin()) {
			*history.begin() = *historyIterator;
			historyIterator = history.begin();
		}
		historyIterator->insert(commandCurPos, 1, c);
		++commandCurPos;
		break;
	}

	commandW->redraw();
}

void UI::handleCommand(const std::string &command) {

	std::pair<std::string::size_type, std::string::size_type> pos
		= nextToken(command);
	std::string::size_type len = pos.second - pos.first;

	/*
	messageW->printf("UI::handleCommand(%s)\n", command.data());
	*/

	if (!len) {
		return;
	}

	std::string data(command, pos.first, len);
	if (command[pos.first] != '/') {
		pos.second = pos.first;
		if (! chatNetwork.length()) {
			messageW->printf("Message not sent, use /chat or /msg command\n");
			messageW->refresh();
			return;
		}

		if (! userExists(chatUser, chatNetwork)) {
			messageW->printf("Message not sent, user not found\n");
			messageW->refresh();
			return;
		}

		sendSignal("/net/msg/send", chatNetwork,
				   new sig::MessageData(chatUser,
									std::string(command, pos.first), 0));

	}

	if (len == 5 && (data == "/quit" || data == "/exit")) {
		sendSignal("/core/module/exits", Core::coreName);
		return;
	}

	if ((len == 2 && (data == "/n" || data == ":n"))
	|| (len == 6 && data == "/names")) {
		/* /names command */
		if (networkUsers.empty()) {
			messageW->printf("There are no connected networks\n");
		} else {
			NetworkUsers::iterator nuit;
			sig::UsersListData::Users::iterator uit;
			for(nuit=networkUsers.begin(); nuit!=networkUsers.end(); ++nuit) {
				if (nuit->second->users.empty()) {
					messageW->printf("Network %s has no connected users\n",
					                 nuit->first.c_str());
				} else {
					messageW->printf("Connected users in network %s:\n",
					                 nuit->first.c_str());
					for(uit=nuit->second->users.begin();
					    uit!=nuit->second->users.end();
					    ++uit) {
						messageW->printf("%s (%s) (%s)\n",
						          uit->second->name.c_str(),
						          uit->first.toString().c_str(),
						          uit->second->status.toString().c_str()
						);
					}
				}
			}
		}
	}

	if (len == 5 && data == "/chat") {

		/* trim */
		pos = nextToken(command, pos.second);
		if (pos.first == std::string::npos) {
			/* end chat */
			messageW->printf("Chat with %s (network %s) finished\n",
			                chatUser.toString().c_str(),
							chatNetwork.c_str());
			chatNetwork.clear();
			messageW->refresh();
			return;
		}

		std::string userString(command, pos.first, pos.second-pos.first);

		std::multimap< std::string, User* > usersfound;
		std::multimap< std::string, User* >::iterator it;
		findUsers(userString, usersfound);

		if(usersfound.size() == 0) {
			messageW->printf("No users match to '%s', try /names command.\n", userString.c_str());
		} else if(usersfound.size() > 1) {
			int iii;
			messageW->printf("Ambiguous user '%s', possible matches:\n",
			                 userString.c_str());
			for(iii=0, it=usersfound.begin(); it!=usersfound.end(); ++iii, ++it) {
				messageW->printf("[#%d] %s\n", iii, it->second->id.toString().c_str());
			}
		} else {

			it = usersfound.begin();
			chatUser = it->second->id;
			chatNetwork = it->first;
			messageW->printf("Chatting with %s (network %s)\n",
			                 chatUser.toString().c_str(),
							 chatNetwork.c_str());
			messageW->refresh();
		}
	}

	if (len == 1 || (len == 4 && data == "/msg") ||
	    (len == 3 && data == "/me")) {
		/* if len == 0 then data == "/" */

		/* trim */
		pos = nextToken(command, pos.second);
		if (pos.first == std::string::npos) {
			/* refuse to send empty line */
			return;
		}

		std::string userString(command, pos.first, pos.second-pos.first);
		pos = nextToken(command, pos.second);
		if (pos.first == std::string::npos) {
			/* no message? */
			return;
		}
		std::string msgString(command, pos.first, pos.second-pos.first);

		std::multimap< std::string, User* > usersfound;
		std::multimap< std::string, User* >::iterator it;
		findUsers(userString, usersfound);

		if(usersfound.size() == 0) {
			messageW->printf("No users match to '%s', try /names command.\n", userString.c_str());
		} else if(usersfound.size() > 1) {
			int iii;
			messageW->printf("Ambiguous user '%s', possible matches:\n",
			                 userString.c_str());
			for(iii=0, it=usersfound.begin(); it!=usersfound.end(); ++iii, ++it) {
				messageW->printf("[#%d] %s\n", iii, it->second->id.toString().c_str());
			}
		} else {

			it = usersfound.begin();
			std::string net = it->first;
			sendSignal("/net/msg/send", net,
			           new sig::MessageData(it->second->id,
		                                std::string(command, pos.first),
		                                len==3?sig::MessageData::ACTION:0));
		}

	} else if (len == 3 && data == "/aw") {
		/* dirty trick */
		pos.second = pos.first + 1;
		goto status;

	} else if (len == 7 && data == "/status") {
	status:
		enum User::State state;
		bool valid;

		pos = nextToken(command, pos.second);
		if (pos.first == std::string::npos) {
			return;
		}

		data.assign(command, pos.first, pos.second - pos.first);
		state = User::getState(data, valid);
		if (!valid) {
			return;
		}

		/* send to all networks */
		std::string net = "/net/";
		pos = nextToken(command, pos.second);
		if (pos.first == std::string::npos) {
			data.clear();
		} else {
			data.assign(command, pos.first, std::string::npos);
		}

		/*
		 * in UserData we must supply valid data only for these
		 * information which has changed (status and/or displayname)
		 */
		sendSignal("/net/status/change", net,
		           new sig::UserData(User("dummy", Address(),
		                                  User::Status(state, data)),
		                             sig::UserData::STATE |
		                             sig::UserData::MESSAGE));
	} else if((len == 3 && data == "/dn")
	|| (len = 12 && data == "/displayname")) {



	} else if(len == 8 && data == "/history") {
		std::list<std::string>::iterator hi;
		int i;

		messageW->printf("Command history:\n");
		for(hi=history.begin(), ++hi, i=1; hi!=history.end(); ++hi, ++i) {
			messageW->printf("[%2d] %s\n", i, hi->c_str());
		}
		messageW->refresh();


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



std::string UI::userName(const std::string &network, const User::ID &id) {
	NetworkUsers::iterator nit = networkUsers.find(network);
	if (nit == networkUsers.end()) {
	not_found:
		sendSignal("/net/conn/are-you-connected", network);
		return id.toString();
	} else {
		sig::UsersListData::Users::iterator uit = nit->second->users.find(id);
		if (uit == nit->second->users.end()) {
			goto not_found;
		} else {
			return uit->second->formattedName();
		}
	}
}


std::string UI::ourUserName(const std::string &network) {
	NetworkUsers::iterator nit = networkUsers.find(network);
	if (nit == networkUsers.end()) {
		sendSignal("/net/conn/are-you-connected", network);
		return "I";
	} else {
		User &ourUser = nit->second->ourUser;
		std::string result(ourUser.name);
		if (!User::nameMatchesNick(ourUser.name, ourUser.id.nick)) {
			result += '(';
			result += ourUser.id.nick;
			result += ')';
		}
		return  result;
	}
}



int UI::findUsers(const std::string &uri, std::multimap< std::string, User* > &map) {

	std::string::size_type len = uri.length();
	std::map<std::string,shared_obj<sig::UsersListData> >::iterator nuit;
	std::map<User::ID, User *>::iterator uit;
	for(nuit=networkUsers.begin(); nuit!=networkUsers.end(); ++nuit) {
		for(uit=nuit->second->users.begin();
			uit!=nuit->second->users.end();
			++uit) {
			if(uit->first.toString().compare(0, len, uri) == 0) {
				map.insert(std::make_pair(nuit->first, uit->second));
			}
		}
	}

	return map.size();
}

bool UI::userExists(const User::ID &user, const std::string &network) {
	std::map<std::string,shared_obj<sig::UsersListData> >::iterator nuit;
	std::map<User::ID, User *>::iterator uit;
	nuit = networkUsers.find(network);
	if (nuit == networkUsers.end()) {
		return false;
	}
	
	uit = nuit->second->users.find(user);
	if (uit == nuit->second->users.end()) {
		return false;
	}

	return true;
}

void UI::handleSigStatusChanged(const std::string &network,
                                const sig::UserData &data) {
	(void)network;

	if (data.flags & sig::UserData::CONNECTED) {
		/*
		messageW->printf("--- %s has connected\n",
		                 data.user.formattedName().c_str());
		*/
		/* At this point user's status is offline, so do nothing more.
		   When user changes status we'll get another signal. */
		return;
	}

	if (data.flags & sig::UserData::DISCONNECTED &&
	    data.user.status.state == User::OFFLINE) {
		return;
	}

	if (data.flags & sig::UserData::NAME) {
		messageW->printf("--- %s is known as %s\n",
		                 data.user.id.toString().c_str(),
		                 data.user.name.c_str());
	}
	if (data.flags == sig::UserData::NAME) {
		return;
	}

	messageW->printf("--- %s ", data.user.formattedName().c_str());
	if (data.flags & sig::UserData::DISCONNECTED) {
		messageW->printf("has disconnected");
	} else {
		messageW->printf("is %s", User::stateName(data.user.status.state));
	}

	if (data.user.status.message.empty()) {
		messageW->printf("\n");
	} else {
		messageW->printf(" (%s)\n", data.user.status.message.c_str());
	}
}



/*
 * --------------------------------------------------------------------------
 *  UI::Window
 * --------------------------------------------------------------------------
 */

void UI::Window::redraw() {
	mvwaddstr(wp, cY, 0, ui->historyIterator->c_str());
	wclrtoeol(wp);
	wmove(wp, cY, ui->commandCurPos);
	wnoutrefresh(wp);
	doupdate();
}

void UI::Window::refresh(int update) {
	wnoutrefresh(wp);
	if (update) {
		doupdate();
	}
}

/*
 * --------------------------------------------------------------------------
 *  UI::OutputWindow
 * --------------------------------------------------------------------------
 */

UI::OutputWindow::OutputWindow(UI *assocUI, unsigned lines, unsigned cols,
                               unsigned starty, unsigned startx)
	: Window(assocUI, lines, cols, starty, startx),
	  buffersize(PPC_UI_OUTPUTWINDOW_BUFFERSIZE),
	  buffer(new char [PPC_UI_OUTPUTWINDOW_BUFFERSIZE]) {
	scrollok(wp, true);
}

int UI::OutputWindow::printf(const char *format, ...) {
	va_list ap;
	int result;
	char *start, *end;
	bool endOfLineEndOfScreen = false;

	va_start(ap, format);
	result = vsnprintf(buffer, buffersize, format, ap);
	va_end(ap);

	if (result < 0) {
		/* FIXME: better error reporting */
		wprintw(wp, "Error in vsnprintf\n");
	}

	waddstr(wp, buffer);
	return 0;

	/* TODO: test this one */
	/*
	 * This fixes the potential bug, where the string itself fits the width
	 * of the window, but the ending newline does not.
	 * This issue may only appear in the last line printed
	 */
	int buflen = strlen(buffer);
	if (buflen > 0 && buffer[buflen-1] == '\n') {
		buffer[buflen-1] = '\0';
		endOfLineEndOfScreen = true;
	}

	/* find how many characters are before newline or end of string */
	std::list<std::pair<const char*, unsigned> > v;
	start = buffer;
	for (; (end = strchr(start, '\n')); start = end + 1) {
		v.push_back(std::make_pair(start, (unsigned)(end-start)));
	}
	if (*start) {
		v.push_back(std::make_pair(start, strlen(start)));
	}

	std::list<std::pair<const char*, unsigned> >::iterator i;
	int j;

	i = v.begin();

	if (i->second > ncols-cX) {
		v.insert(i, 1, std::make_pair(i->first, ncols-cX));
		i->first += ncols-cX;
		i->second -= ncols-cX;
	}

	for(i=v.begin(); i!=v.end(); ++i) {

		while(i->second > ncols) {
			v.insert(i, 1, std::make_pair(i->first, ncols));
			i->first += ncols;
			i->second -= ncols;
		}
	}

	/*
	 * now we know that we need v.size() lines to print, each element
	 * of the list contains pointer of the beginning of the line
	 * and number of characters to print in that line
	 */

	if (v.size() >= nlines) {
		int linesToDelete = v.size() - nlines;
		while(linesToDelete) {
			v.pop_front();
			--linesToDelete;
		}
		wclear(wp);
		for(i=v.begin(), j=0; i!=v.end(); ++i, ++j) {
			mvwaddnstr(wp, j, 0, i->first, i->second);
		}
	} else {
		int size = v.size();
		int linesleft = nlines-cY;
		if (size > linesleft) {
			wscrl(wp, size - linesleft);
			cY -= size - linesleft;
		}
		wmove(wp, cY, cX);
		i=v.begin();
		waddnstr(wp, i->first, i->second);
		wclrtoeol(wp);
		while(++i!=v.end()) {
			mvwaddnstr(wp, ++cY, 0, i->first, i->second);
			wclrtoeol(wp);
		}
	}

	if (endOfLineEndOfScreen) {
		cY = nlines;
		cX = ncols;
		wmove(wp, cY, cX);
	} else {
		getyx(wp, cY, cX);
	}

	if ((size_t)result >= buffersize) {
		/* FIXME: better error reporting */
		wprintw(wp, "Warning! Output truncated!\n");
	}

	return 0;
}


}
