##
## Makefile
## $Id: Makefile,v 1.1 2007/12/03 14:46:33 mina86 Exp $
##

HPP_FILES	:= $(wildcard *.hpp)
CPP_FILES	:= $(wildcard *.cpp)
OBJ_FILES	:= $(addsuffix .o,$(basename $(CPP_FILES)))

all: $(OBJ_FILES)

%.o: %.cpp $(HPP_FILES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

doc:
	rm -rf -- doc/html
	doxygen

xml-test: xml-test.o xml-parser.o
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -rf -- *.o xml-test

.PHONY: all doc clean
