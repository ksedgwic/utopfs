ROOTDIR = ../..

include $(ROOTDIR)/config/define.mk

LIBSRC =	\
			pydirentryfunc.cpp \
			pyblockstore.cpp \
			pyfilesystem.cpp \
			pystat.cpp \
			pyutpinit.cpp \
			statemap.cpp \
			$(NULL)

MODSO =		_utp

DEFS +=		-DPYUTP_IMPL

include $(ROOTDIR)/config/depend.mk

# Dependencies
include $(ROOTDIR)/libutp/src/export.mk

# External Package Dependencies
include $(ROOTDIR)/config/extdep/python.mk
include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End: