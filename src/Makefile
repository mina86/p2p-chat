##
## Makefile
## $Id: Makefile,v 1.2 2007/12/08 18:02:11 mina86 Exp $
##

HPP_FILES	:= $(wildcard *.hpp)
OBJ_FILES	:= $(addsuffix .o,$(basename $(wildcard *.cpp)))

all: $(OBJ_FILES)

%.o: %.cpp $(HPP_FILES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

doc:
	rm -rf -- doc/html
	doxygen

tests:
	cd tests && exec $(MAKE)

clean:
	exec rm -rf -- *.o xml-test
	cd tests && exec $(MAKE) clean

.PHONY: all doc tests clean
