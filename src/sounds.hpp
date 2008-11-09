/** \file
 * An "User interface" module playing sounds.
 * Copyright 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
