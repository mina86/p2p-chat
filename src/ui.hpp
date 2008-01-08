/** \file
 * User interface header file.
 * $Id: ui.hpp,v 1.8 2008/01/08 03:15:06 mco Exp $
 */

#ifndef H_UI_HPP
#define H_UI_HPP

#define UI_HISTORY_SIZE		100

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
	 * Returns a next string token from given string at given
	 * position.  When there are no more tokens a pair of two
	 * std::string::npos values is returned.
	 *
	 * \param str string to parse.
	 * \param pos index to find searching for tokens.
	 * \return an index of first and one-past-the-last char of token.
	 */
	std::pair<std::string::size_type, std::string::size_type>
	nextToken(const std::string &str, std::string::size_type pos = 0);

	/**
	 * commands history buffer
	 * first entry is current command buffer
	 */
	std::list<std::string> history;
	std::list<std::string>::iterator historyIterator;

	unsigned int commandCurPos;

	/** screen size */
	int maxY;
	int maxX;

	/** command window identifier */
	Window *commandW;

	/** status window identifier */
	WINDOW *statusW;

	/** message window identifier */
	WINDOW *messageW;

	/** test window */
	OutputWindow *testW;

	struct Window {

		Window(UI *assocUI, int nlines, int ncols, int starty, int startx);
		~Window();

		/** clears the window and writes (part of) command buffer */
		void redraw();

		/** refreshes window */
		void refresh(int update=0);

	protected:
		/** assiociated UI object */
		UI *ui;

		/** this window's ncurses window pointer */
		WINDOW *wp;

		/** geometry of windows, similar to ncurses' newwin */
		int starty, startx;
		int nlines, ncols;

		/** position of cursor */
		int cY;
		int cX; /* unused, see ui->commandCurPos */
	};

	struct CommandWindow : Window {

		CommandWindow(UI *assocUI, int nlines, int ncols, int starty, int startx);
	};

	struct OutputWindow : Window {

		OutputWindow(UI *assocUI, int nlines, int ncols, int starty, int startx);
		~OutputWindow();

		/* printf-like function to output characters in a controlled manner */
		int printf(WINDOW *wm, const char *format, ...);

	protected:
		/* internal buffer */
		char *buffer;
		size_t buffersize;

	};

};


}

#endif
