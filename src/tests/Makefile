##
## Makefile
## $Id: Makefile,v 1.1 2007/12/08 18:02:11 mina86 Exp $
##

HPP_FILES	:= $(wildcard ../*.hpp)
EXE_FILES	:= $(basename $(wildcard *.cpp))

all: $(EXE_FILES)

run: all
	@echo shared_obj test; exec ./shared-obj
	@echo XML tokenizer and parser test; exec ./xml-parser <data.xml

../%.o:
	cd .. && exec make $@

%.o: %.cpp $(HPP_FILES)
	exec $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

xml-parser: xml-parser.o ../xml-parser.o
	exec $(CXX) $(LDFLAGS) -o $@ $^

shared-obj: shared-obj.cpp ../shared-obj.hpp
	exec $(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

clean:
	exec rm -rf -- *.o $(EXE_FILES)

.PHONY: all run clean ../%.o
