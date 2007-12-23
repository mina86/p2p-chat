##
## Makefile
## $Id: Makefile,v 1.4 2007/12/23 00:52:09 mina86 Exp $
##

CXXFLAGS	+= -fno-rtti -fno-enforce-eh-specs -fno-threadsafe-statics

HPP_FILES	:= $(wildcard ../*.hpp)
EXE_FILES	:= $(basename $(wildcard *.cpp) $(wildcard *.c))

all: $(EXE_FILES)

run: all
	@echo shared_obj test; exec ./shared-obj
	@echo XML tokenizer and parser test; exec ./xml-parser <data.xml

../%.o: ../%.cpp $(HPP_FILES)
	exec $(MAKE) -C.. $(notdir $@)

%.o: %.cpp $(HPP_FILES)
	exec $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

xml-parser: xml-parser.o ../xml-parser.o ../ppcp-parser.o ../user.o
	exec $(CXX) $(LDFLAGS) -o $@ $^

shared-obj: shared-obj.cpp ../shared-obj.hpp
	exec $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

multicast: multicast.c
	exec $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	exec rm -rf -- *.o $(EXE_FILES)

.PHONY: all run clean
