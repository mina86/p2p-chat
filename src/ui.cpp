/** \file
 * User interface implementation.
 * $Id: ui.cpp,v 1.18 2008/01/08 14:55:15 mco Exp $
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "io.hpp"
#include "ui.hpp"

#define PPC_UI_OUTPUTWINDOW_BUFFERSIZE	4096

namespace ppc {


/**
 * A private (file-scope) variable to make sequential numbers in
 * module names.  Each file implementing each module (should) have its
 * own \a seq variable.
 */
static unsigned long seq = 0;


UI::UI(Core &c, int infd /* some more arguments */)
	: Module(c, "/ui/mco/", seq++), stdin_fd(infd) {

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
	messageW->printf("backspace, delchar, erasechar: %d, %d, %d, \n", KEY_BACKSPACE, KEY_DC, erasechar());
	messageW->printf("UP, DOWN, LEFT, RIGHT: %d %d %d %d\n", KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT);
	wprintw(statusW, "W trakcie tworzenia");
	wnoutrefresh(stdscr);
	messageW->refresh();
	wnoutrefresh(statusW);
	commandW->refresh();
	doupdate();
}


UI::~UI() {
	/* clear command history */
	history.clear();
	/* destroy windows */
	delete messageW;
	delwin(statusW);
	delete commandW;

	/* whatever needed */
	endwin();
	printf("UI exiting\n");
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
		const sig::UserData &data = *sig.getData<sig::UserData>();
		(void)data; /* to silence warning for the time being */


	} else if (sig.getType() == "/net/msg/got") {
		/* we have recieved a message (display it maybe?) */
		const sig::MessageData &data = *sig.getData<sig::MessageData>();
		(void)data; /* to silence warning for the time being */

	} else if (sig.getType() == "/net/msg/sent") {
		/* a message have been sent (display it maybe?) */
		const sig::MessageData &data = *sig.getData<sig::MessageData>();
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
	mvwprintw(statusW, 0, 0, "Entered character: [O:%o] [D:%d] [H:%x]", c, c, c);
	wclrtoeol(statusW);
	wnoutrefresh(statusW);
	doupdate();
	std::list<std::string>::iterator tmpHistoryIterator;
	switch(c) {
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
			history.push_front(std::string(""));
			historyIterator = history.begin();
			commandCurPos = 0;
			if(history.size() > UI_HISTORY_SIZE) {
				history.pop_back();
			}
			commandW->redraw();
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
				commandW->redraw();
			}
			break;
		/* delete key */
		case 0x04:
		case KEY_DC:
			if(commandCurPos < historyIterator->length()) {
				historyIterator->erase(commandCurPos, 1);
				commandW->redraw();
			}
			break;
		case 0x10:
		case KEY_UP:
			tmpHistoryIterator = historyIterator;
			++tmpHistoryIterator;
			if(tmpHistoryIterator != history.end()) {
				historyIterator=tmpHistoryIterator;
				commandCurPos = historyIterator->length();
				commandW->redraw();
			}
			break;
		case 0x0e:
		case KEY_DOWN:
			if(historyIterator != history.begin()) {
				--historyIterator;
				commandCurPos = historyIterator->length();
				commandW->redraw();
			}
			break;
		case 0x02:
		case KEY_LEFT:
			if(commandCurPos>0) {
				--commandCurPos;
			}
			commandW->redraw();
			break;
		case 0x06:
		case KEY_RIGHT:
			if(commandCurPos < historyIterator->size()) {
				++commandCurPos;
			}
			commandW->redraw();
			break;
		case 0x01:
		case KEY_HOME:
			commandCurPos = 0;
			commandW->redraw();
			break;
		case 0x05:
		case KEY_END:
			commandCurPos = historyIterator->size();
			commandW->redraw();
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
			commandW->redraw();
			break;
	}
}

void UI::handleCommand(const std::string &command) {

	std::pair<std::string::size_type, std::string::size_type> pos
		= nextToken(command);
	std::string::size_type len = pos.second - pos.first;

	messageW->printf("UI::handleCommand(%s)\n", command.data());
	messageW->refresh();

	if (!len) {
		return;
	}

	std::string data(command, pos.first, len);
	/*
	if (command[pos.first] != '/') {
		pos.second = pos.first;
		goto message;
	}
	*/

	if (len == 5 && (data == "/quit" || data == "/exit")) {
		sendSignal("/core/module/exits", Core::coreName);
		return;
	}

	if (len == 2 && data == "ll") {
		std::list<std::string>::iterator i;
		int j;
		for(i=history.begin(), j=0; i!=history.end(); ++i, ++j) {
			messageW->printf("[%3d]: %s\n", j, i->c_str());
			messageW->refresh();
		}
	}

	return;

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

/*
 * --------------------------------------------------------------------------
 *  UI::Window
 * --------------------------------------------------------------------------
 */

UI::Window::Window(UI *assocUI, int nlines, int ncols, int starty,
	int startx)
	: nlines(nlines), ncols(ncols), starty(starty), startx(startx),
	  ui(assocUI), cY(0), cX(0) {
	
	wp = newwin(nlines, ncols, starty, startx);
}

UI::Window::~Window() {
	delwin(wp);
}

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
 *  UI::CommandWindow
 * --------------------------------------------------------------------------
 */

UI::CommandWindow::CommandWindow(UI *assocUI, int nlines, int ncols,
	int starty, int startx)
	: Window(assocUI, nlines, ncols, starty, startx) {

}

/*
 * --------------------------------------------------------------------------
 *  UI::OutputWindow
 * --------------------------------------------------------------------------
 */

UI::OutputWindow::OutputWindow(UI *assocUI, int nlines, int ncols,
	int starty, int startx)
	: Window(assocUI, nlines, ncols, starty, startx) {

	buffersize = PPC_UI_OUTPUTWINDOW_BUFFERSIZE;
	buffer = new char [buffersize];

	scrollok(wp, true);
}

UI::OutputWindow::~OutputWindow() {
	delete buffer;
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

	/* TODO: test this one */
	/* 
	 * This fixes the potential bug, where the string itself fits the width
	 * of the window, but the ending newline does not.
	 * This issue may only appear in the last line printed
	 */
	int buflen = strlen(buffer);
	if (buflen > 0 && buffer[buflen-1] == 'N') {
		buffer[buflen-1] = '\0';
		endOfLineEndOfScreen = true;
	}

	/* find how many characters are before newline or end of string */
	std::list<std::pair<char*, int> > v;
	start = buffer;
	/* end = strchr(start, '\n'); */
	end = strchr(start, 'N');
	while(end != NULL) {
		v.push_back(std::make_pair(start, (int)(end-start)));
		start = end+1;
		end = strchr(start, 'N');
	}
	v.push_back(std::make_pair(start, strlen(start)));

	std::list<std::pair<char*, int> >::iterator i;
	int j;

	i=v.begin();

	if(i->second > ncols-cX) {
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
		char *ifirst;
		int isecond;
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

	if(result >= buffersize) {
		/* FIXME: better error reporting */
		wprintw(wp, "Warning! Output truncated!\n");
	}

	return 0;
}


}
