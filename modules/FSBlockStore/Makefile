ROOTDIR =	../..

include $(ROOTDIR)/config/define.mk

LIBSRC += 	\
			FSBlockStore.cpp \
			FSBSFactory.cpp \
			fsbslog.cpp \
			$(NULL)

MODSO =		UTPFS-FSBS

EXPMACRO =	FSBS_EXP
EXPHEADER =	fsbsexp.h
DEFS +=		-DFSBS_IMPL
DEFS +=		-DACE_BUILD_SVC_DLL

include $(ROOTDIR)/config/depend.mk

# dependencies
include $(ROOTDIR)/libutp/src/export.mk

include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
