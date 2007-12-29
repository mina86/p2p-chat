/** \file
 * User interface header file.
 * $Id: ui.hpp,v 1.1 2007/12/29 02:39:04 mina86 Exp $
 */

#ifndef H_UI_HPP
#define H_UI_HPP

#include <map>

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

	NetworkUsers networkUsers;
};


}

#endif
