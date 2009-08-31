ifndef ACE_mk__
       ACE_mk__ = 1

ifeq ($(SYSNAME),Darwin)
ACEROOT =		/usr/local/ACE_wrappers

INCS +=			-I$(ACEROOT)
LIBS +=			-L$(ACEROOT)/lib
ENVLDLIBPATH +=	$(ACEROOT)/lib

# List the libraries after the optional -L above ...
LIBS +=			\
				-lACEXML_Parser \
				-lACEXML \
				-lACE \
				$(NULL)
endif

ifeq ($(SYSNAME),Linux)

# Prefer RPM installed ACE in the system places (/usr/include/ace, etc).
#
# Failing that use ACE_ROOT if set.
#
# If ACE_ROOT is not set, use /usr/local/ACE_wrappers

ifneq "" "$(wildcard /usr/include/ace/Version.h)"
# It is in the standard system places.
else
# It is not in the standard places, set include and library paths to
# ACE_ROOT, or /usr/local/ACE_wrappers by default.
ifdef ACE_ROOT
ACEROOT =		$(ACE_ROOT)
else
ACEROOT =		/usr/local/ACE_wrappers
endif
INCS +=			-I$(ACEROOT)
LIBS +=			-L$(ACEROOT)/lib
ENVLDLIBPATH +=	$(ACEROOT)/lib
endif
# List the libraries after the optional -L above ...
LIBS +=			\
				-lACEXML_Parser \
				-lACEXML \
				-lACE \
				$(NULL)
endif

ifeq ($(SYSNAME),WIN32)
# On WIN32 we assume that ACE is installed in C:\ACE_wrappers.  If it
# is installed somewhere else the env variable "ACE_ROOT" should be
# pointing at it.
ifdef ACE_ROOT
ACEROOT =		$(shell cygpath -m $(ACE_ROOT))
else
ACEROOT =		$(shell cygpath -m $(ABSDIST))
endif
ifeq ($(BUILD),DEBUG)
LIBSFX3 = 		d
endif
INCS +=			-I$(ACEROOT)/include
LIBS +=			$(ACEROOT)/lib/ace$(LIBSFX3).lib \
				$(ACEROOT)/lib/ACEXML_Parser$(LIBSFX3).lib \
				$(ACEROOT)/lib/ACEXML$(LIBSFX3).lib \
				$(NULL)
ENVLDLIBPATH +=	$(shell cygpath $(ACEROOT)/lib)
endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # ACE_mk__
