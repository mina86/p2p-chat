/** \file
 * An "User interface" module playing sounds.
 * $Id: sounds.hpp,v 1.1 2008/01/23 03:02:45 mina86 Exp $
 */

#ifndef H_SOUNDS_HPP
#define H_SOUNDS_HPP

#include "application.hpp"


namespace ppc {

struct SoundsUI : public Module {
	/**
	 * Creates sounds user interface object.
	 * \param c core module.
	 */
	SoundsUI(Core &c) : Module(c, "/ui/sounds/", seq++) { }

	virtual int setFDSets(fd_set *rd, fd_set *wr, fd_set *ex);
	virtual int doFDs(int nfds, const fd_set *rd, const fd_set *wr,
	                  const fd_set *ex);
	virtual void recievedSignal(const Signal &sig);

private:
	/** Variable to make sequential numbers in module names. */
	static unsigned seq;
};

}

#endif
