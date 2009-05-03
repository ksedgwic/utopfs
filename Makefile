# This is a comment.

CXXFLAGS +=	-g -Wall -Werror

DEFS +=		-D_FILE_OFFSET_BITS=64

LIBS +=		-lfuse

PRGEXE =	utopfs

PRGSRC =	utopfs.cpp

PRGOBJ =	$(PRGSRC:%.cpp=%.o)

all:		$(PRGEXE)

$(PRGEXE):		$(PRGOBJ)
	$(CXX) -o $@ $(PRGOBJ) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(PRGEXE) $(PRGOBJ)

%.o:		%.cpp
	$(CXX) -c $< $(CPPFLAGS) $(CXXFLAGS) $(DEFS)
