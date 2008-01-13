/** \file
 * User interface header file.
 * $Id: ui.hpp,v 1.12 2008/01/13 12:16:21 mina86 Exp $
 */

#ifndef H_UI_HPP
#define H_UI_HPP

#include <map>
#include <list>
#include <ncurses.h>

#include "application.hpp"


namespace ppc {

struct UI : public Module {
	/**
	 * Craetes new user interface module.  UI modules are named \c
	 * /ui/mco/number where number is a sequence number starting from
	 * zero.
	 *
	 * \param core core module.
	 * \param infd file descriptor used to read commands.
	 */
	UI(Core &core, int infd = 0 /* some more arguments */);

	/** Frees all resources. */
	~UI();

	virtual int setFDSets(fd_set *rd, fd_set *wr, fd_set *ex);
	virtual int doFDs(int nfds, const fd_set *rd, const fd_set *wr,
	                  const fd_set *ex);
	virtual void recievedSignal(const Signal &sig);


private:
	struct Window;
	struct CommandWindow;
	struct OutputWindow;

	/** A list of users in each network. */
	typedef std::map<std::string,shared_obj<sig::UsersListData> >NetworkUsers;

	/** A "standard input" file descriptor. */
	int stdin_fd;

	/** A list of users in each network. */
	NetworkUsers networkUsers;

	/**
	 * Returns a string representing given user.
	 *
	 * \param network name of network module.
	 * \param id      user's ID.
	 */
	std::string userName(const std::string &network, const User::ID &id);

	/**
	 * Returns a string representing our user.
	 *
	 * \param network name of network module.
	 */
	std::string ourUserName(const std::string &network);

	/**
	 * Finds user by given URI
	 * \param uri user uri (or starting part of it)
	 * \param up array of pointers to User
	 * \param n find at most that many users
	 * \return number of users found (it could be more than n)
	 */
	int findUsers(const std::string &uri, User **up, int n);

	/**
	 * Handles every single character received from user
	 * \param c character received
	 */
	void handleCharacter(int c);

	/**
	 * Handles a command user enterd.
	 * \param command command user entered.
	 */
	void handleCommand(const std::string &command);

	/**
	 * Handles \c /net/status/changed signal.
	 * \param network name of network module which sent signal.
	 * \param data    signal's argument.
	 */
	void handleSigStatusChanged(const std::string &network,
	                            const sig::UserData &data);


	/**
	 * Returns a next string token from given string at given
	 * position.  When there are no more tokens a pair of two
	 * std::string::npos values is returned.
	 *
	 * \param str string to parse.
	 * \param pos index to find searching for tokens.
	 * \return an index of first and one-past-the-last char of token.
	 */
	static std::pair<std::string::size_type, std::string::size_type>
	nextToken(const std::string &str, std::string::size_type pos = 0);


	/**
	 * Commands history buffer.
	 * First entry is current command buffer.
	 */
	std::list<std::string> history;
	std::list<std::string>::iterator historyIterator;

	unsigned int commandCurPos;

	/** Screen height. */
	unsigned maxY;
	/** Screen width. */
	unsigned maxX;

	/** command window identifier */
	Window *commandW;

	/** status window identifier */
	WINDOW *statusW;

	/** message window identifier */
	OutputWindow *messageW;

	struct Window {

		Window(UI *assocUI, unsigned lines, unsigned cols,
		       unsigned starty, unsigned startx)
			: ui(assocUI), nlines(lines), ncols(cols), cY(0), cX(0) {
			wp = newwin(nlines, ncols, starty, startx);
		}

		~Window() {
			delwin(wp);
		}

		/** clears the window and writes (part of) command buffer */
		void redraw();

		/** refreshes window */
		void refresh(int update = 0);

	protected:
		/** assiociated UI object */
		UI *ui;

		/** this window's ncurses window pointer */
		WINDOW *wp;

		/** Window height. */
		unsigned nlines;
		/** Window's width. */
		unsigned ncols;

		/** position of cursor */
		int cY;
		int cX; /* unused, see ui->commandCurPos */
	};

	struct CommandWindow : Window {

		CommandWindow(UI *assocUI, unsigned lines, unsigned cols,
		              unsigned starty, unsigned startx)
			: Window(assocUI, lines, cols, starty, startx) { }
	};

	struct OutputWindow : Window {

		OutputWindow(UI *assocUI, unsigned nlines, unsigned ncols,
		             unsigned starty, unsigned startx);
		~OutputWindow() {
			delete buffer;
		}

		/* printf-like function to output characters in a controlled manner */
		int printf(const char *format, ...);

	protected:
		/* internal buffer */
		size_t buffersize;
		char *buffer;

	};

};


}

#endif
