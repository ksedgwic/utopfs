ROOTDIR =	../..

include $(ROOTDIR)/config/define.mk

PROTO +=	\
			Directory.proto \
			INode.proto \
			$(NULL)

LIBSRC += 	\
			BlockNode.cpp \
			DirNode.cpp \
			FileNode.cpp \
			RootDirNode.cpp \
			SpecialDirNode.cpp \
			SpecialFileNode.cpp \
			UTFileSystem.cpp \
			utfslog.cpp \
			UTFSSvc.cpp \
			$(NULL)

MODSO =		UTPFS-UTFS

EXPMACRO =	UTFS_EXP
EXPHEADER =	utfsexp.h
DEFS +=		-DUTFS_IMPL
DEFS +=		-DACE_BUILD_SVC_DLL

include $(ROOTDIR)/config/depend.mk

# dependencies
include $(ROOTDIR)/libutp/src/export.mk

include $(ROOTDIR)/config/extdep/protobuf.mk
include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
