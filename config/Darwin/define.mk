# ----------------------------------------------------------------
# This section determines if we have a new version of GCC
ifeq ($(CXX),insure)
  # insure does not pass through the -dumpversion option.
  CXX_FOR_VERSION_TEST = g++
else
  CXX_FOR_VERSION_TEST = $(CXX)
endif

CXX_VERSION := $(shell $(CXX_FOR_VERSION_TEST) -dumpversion)
ifeq (cmd,$(findstring cmd,$(SHELL)))
CXX_MAJOR_VERSION := $(shell $(CXX_FOR_VERSION_TEST) -dumpversion | sed -e "s/[^0-9\.]//g" | sed -e "s/\..*$$//")
else
CXX_MAJOR_VERSION := $(shell $(CXX_FOR_VERSION_TEST) -dumpversion | sed -e 's/[^0-9\.]//g' | sed -e 's/\..*$$//')
endif
ifeq ($(findstring $(CXX_MAJOR_VERSION),1 2 3),$(CXX_MAJOR_VERSION))
GXX_4_OR_BETTER := 0
else
GXX_4_OR_BETTER := 1
endif
# ----------------------------------------------------------------

ABSROOT =		$(shell cd $(ROOTDIR) && pwd)
CURRDIRU =		$(shell pwd)
CURRDIRW =		$(CURRDIRU)
CURRDIRw =		$(CURRDIRU)

LIBEXT =        .a
LIBCMD =        ar cr $@

OBJEXT =		.o
SOEXT =			.so
EXEEXT =	

CPPCMD =		g++

ifeq ($(BUILD),DEBUG)
CPPFLAGS =		-g -Wall -fPIC
else
CPPFLAGS =		-g -Wall -fPIC -Werror -O3
endif

PROCTYPE = $(shell uname -p)

## Disabling visibility=hidden so backtrace code works
## 
## # Disabling visibility=hidden for opteron to get around
## # GCC bug
## # see: http://benjamin.smedbergs.us/blog/2005-10-27/gcc-40-workaround/
## # see: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=20297
## #
## ifeq (,$(filter-out i686, $(PROCTYPE)))
## ifeq ($(GXX_4_OR_BETTER), 1)
## DEFS += -DGCC_HASCLASSVISIBILITY
## CPPFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
## endif
## endif

ARCHLIB = lib
ifeq (,$(filter-out i686, $(PROCTYPE)))
CPPFLAGS += -march=i686
endif
ifeq (,$(filter-out x86_64, $(PROCTYPE)))
CPPFLAGS += -march=opteron
ARCHLIB = lib64
endif

LDCMD =			g++
LDFLAGS =		-Xlinker --export-dynamic

SOCMD =			g++
SOFLAGS =		-shared -Xlinker --export-dynamic

INSTFILECMD = 	install -D
INSTDIRCMD =	install -d

SRCENVSH =		env.sh

DEFS +=			-DLINUX -D_REENTRANT
INCS +=			-I. -I$(GENDIR)

# special things we build under Linux
TRGDEP =		$(BLTDEP)
TRGDEPX =		$(BLTDEPX)

# filter out /usr/local/ACE_wrappers from dependencies
# DEPFLT =	/usr/local/ACE_wrappers
DEPFLT =	/xyzzy

BLTMODOPTSO =	$(MODSO:%=$(LIBDIR)/%$(SOEXT))

ifdef USE_VALGRIND
DEFS +=		-DUSE_VALGRIND
endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
