ROOTDIR =	../..

include $(ROOTDIR)/config/define.mk

LIBSRC += 	\
			S3BlockStore.cpp \
			S3BSFactory.cpp \
			s3bslog.cpp \
			$(NULL)

MODSO =		UTPFS-S3BS

EXPMACRO =	S3BS_EXP
EXPHEADER =	s3bsexp.h
DEFS +=		-DS3BS_IMPL
DEFS +=		-DACE_BUILD_SVC_DLL

include $(ROOTDIR)/config/depend.mk

# dependencies
include $(ROOTDIR)/libutp/src/export.mk

# external dependencies
include $(ROOTDIR)/config/extdep/ACE.mk
include $(ROOTDIR)/config/extdep/libs3.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
