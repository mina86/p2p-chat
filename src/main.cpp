/** \file
 * Main file.
 * $Id: main.cpp,v 1.2 2008/01/06 15:24:09 mina86 Exp $
 */

#include <stdio.h>
#include "application.hpp"
#include "ui.hpp"

/** Main function. */
int main(void) {

	struct ppc::Config c;
	struct ppc::Core core(c);
	struct ppc::UI *ui = new ppc::UI(core);
	core.addModule(*ui);
	core.run();
	return 0;
}
