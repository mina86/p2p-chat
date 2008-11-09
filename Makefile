##
## Makefile
##

all:
	exec $(MAKE) -C src all

%:
	exec $(MAKE) -C src $@

