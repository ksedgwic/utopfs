ROOTDIR = ../..

include $(ROOTDIR)/config/define.mk

LIBSRC =	\
			pyblockstore.cpp \
			pybsstat.cpp \
			pydirentryfunc.cpp \
			pyfilesystem.cpp \
			pyshe.cpp \
			pystat.cpp \
			pystatvfs.cpp \
			pyutpinit.cpp \
			pyutplog.cpp \
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
