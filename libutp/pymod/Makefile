ROOTDIR = ../..

include $(ROOTDIR)/config/define.mk

LIBSRC =	\
			pyblockstore.cpp \
			pybsstat.cpp \
			pydirentryfunc.cpp \
			pyshe.cpp \
			pystat.cpp \
			pyutpinit.cpp \
			pyutplog.cpp \
			statemap.cpp \
            pyfilesystem.cpp \
            pystatvfs.cpp \
            $(NULL)

ifneq (WIN32,$(SYSNAME))
MODSO =		_utp
else
ifeq ($(BUILD),DEBUG)
MODSOPYD =	_utp_d
else
MODSOPYD =	_utp
endif
endif

DEFS +=		-DPYUTP_IMPL

ifneq (WIN32,$(SYSNAME))
CPPFLAGS +=	-fno-strict-aliasing
endif

include $(ROOTDIR)/config/depend.mk

# Dependencies
include $(ROOTDIR)/libutp/src/export.mk

# External Package Dependencies
include $(ROOTDIR)/config/extdep/protobuf.mk
include $(ROOTDIR)/config/extdep/python.mk
include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
