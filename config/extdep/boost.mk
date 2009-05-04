ifndef boost_mk__
       boost_mk__ = 1

ifeq ($(SYSNAME),Linux)
LIBS +=			-lboost_filesystem
endif

ifeq ($(SYSNAME),WIN32)
# On WIN32 we assume that boost is installed in
# C:/Boost/include/boost-1_33.  If it is installed somewhere else the
# env variable "BOOST_ROOT" should be pointing at it.
ifdef BOOST_ROOT
BOOSTROOT =		$(shell cygpath -m $(BOOST_ROOT))
else
BOOSTROOT =		c:/Boost
endif

ifndef BOOSTVER
BOOSTVER = 1_33
endif

INCS +=			-I$(BOOSTROOT)

# This is pretty lame ... boot doesn't have a consistent root/include
# root/lib story ... so by adding this second include we can make
# stuff work without hopefully breaking anything else ... sigh.
INCS +=			-I$(BOOSTROOT)/include/boost-$(BOOSTVER)

ifeq ($(BUILD),DEBUG)
LIBSBST = 		gd-$(BOOSTVER)
else 
LIBSBST = 		$(BOOSTVER)
endif

LIBS +=			$(BOOSTROOT)/lib/libboost_filesystem-vc71-mt-$(LIBSBST).lib
endif   # if win32

endif # boost_mk__

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
