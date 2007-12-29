/** \file
 * A shared buffer declaration.  This file is rather small, yes.
 * $Id: shared-buffer.hpp,v 1.1 2007/12/29 02:34:55 mina86 Exp $
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
