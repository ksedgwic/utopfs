ROOTDIR =	../..

include $(ROOTDIR)/config/define.mk

LIBSRC += 	\
			UTFSSvc.cpp \
			utfslog.cpp \
			UTFileSystem.cpp \
			$(NULL)

MODSO =		UTPFS-UTFS

EXPMACRO =	UTFS_EXP
EXPHEADER =	utfsexp.h
DEFS +=		-DUTFS_IMPL
DEFS +=		-DACE_BUILD_SVC_DLL

include $(ROOTDIR)/config/depend.mk

# dependencies
include $(ROOTDIR)/libutp/src/export.mk

include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
