ROOTDIR =	../..

include $(ROOTDIR)/config/define.mk

LIBSRC += 	\
			VBlockStore.cpp \
			VBSChild.cpp \
			VBSFactory.cpp \
			VBSGetRequest.cpp \
			vbslog.cpp \
			VBSPutRequest.cpp \
			VBSRefreshBlockRequest.cpp \
			VBSRefreshFinishRequest.cpp \
			VBSRefreshStartRequest.cpp \
			VBSRequest.cpp \
			$(NULL)

MODSO =		UTPFS-VBS

EXPMACRO =	VBS_EXP
EXPHEADER =	vbsexp.h
DEFS +=		-DVBS_IMPL
DEFS +=		-DACE_BUILD_SVC_DLL

include $(ROOTDIR)/config/depend.mk

# dependencies
include $(ROOTDIR)/libutp/src/export.mk

include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
