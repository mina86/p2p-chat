##
## Makefile
## $Id: Makefile,v 1.5 2007/12/29 02:34:30 mina86 Exp $
##

CXXFLAGS	+= -fno-rtti -fno-enforce-eh-specs -fno-threadsafe-statics

HPP_FILES	:= $(wildcard *.hpp)
OBJ_FILES	:= $(addsuffix .o,$(basename $(wildcard *.cpp)))

all: main

%.o: %.cpp $(HPP_FILES)
	exec $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

main: $(OBJ_FILES)
	exec $(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(OBJ_FILES)

doc:
	exec rm -rf -- doc/html
	exec doxygen

tests:
	cd tests && exec $(MAKE)

clean:
	exec rm -rf -- *.o mail
	cd tests && exec $(MAKE) clean

.PHONY: all doc tests clean
