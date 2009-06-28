ROOTDIR =	../..

include $(ROOTDIR)/config/define.mk

LIBSRC += 	\
			BDBBlockStore.cpp \
			BDBBSFactory.cpp \
			bdbbslog.cpp \
			$(NULL)

MODSO =		UTPFS-BDBBS

EXPMACRO =	BDBBS_EXP
EXPHEADER =	bdbbsexp.h
DEFS +=		-DBDBBS_IMPL
DEFS +=		-DACE_BUILD_SVC_DLL

include $(ROOTDIR)/config/depend.mk

# dependencies
include $(ROOTDIR)/libutp/src/export.mk

include $(ROOTDIR)/config/extdep/ACE.mk
include $(ROOTDIR)/config/extdep/bdb.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
