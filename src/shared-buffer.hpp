/** \file
 * A shared buffer declaration.  This file is rather small, yes.
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

namespace ppc {

/**
 * A static buffer that all functions and methods can for temporary
 * storage.  This may be used to convert an integer into a string
 * without the need to allocate any buffer on stac or wherever.  Our
 * code is not thread safe anyway so we can exploit this fact to speed
 * things up a bit. :) Of course one must know that content of this
 * buffer is undefined after a call to any method from a ppc
 * namespace.
 */
extern char sharedBuffer[1024];

}
