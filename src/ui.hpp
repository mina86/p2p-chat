/** \file
 * User interface header file.
 * $Id: ui.hpp,v 1.5 2008/01/06 21:43:22 mco Exp $
 */

#ifndef H_UI_HPP
#define H_UI_HPP

#define UI_HISTORY_SIZE		100

#include <map>
#include <list>

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

	/** redraw the command window */
	void winCommandRedraw();

	/**
	 * commands history buffer
	 * first entry is current command buffer
	 */
	std::list<std::string> history;
	std::list<std::string>::iterator historyIterator;
};


}

#endif
