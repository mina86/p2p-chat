##
## Makefile
## $Id: Makefile,v 1.1 2007/12/25 01:40:46 mina86 Exp $
##

all:
	exec $(MAKE) -C src all

%:
	exec $(MAKE) -C src $@

