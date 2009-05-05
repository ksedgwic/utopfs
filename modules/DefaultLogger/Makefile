# Copyright (C) 2007 Bonsai Software, Inc.  All rights reserved.

ROOTDIR =	../..

include $(ROOTDIR)/config/define.mk

LIBSRC += 	\
			DFLGSvc.cpp \
			Formatter.cpp \
			Stream.cpp \
			$(NULL)

MODSO =		UTPFS-DFLG

EXPMACRO =	DFLG_EXP
EXPHEADER =	dflgexp.h
DEFS +=		-DDFLG_IMPL
DEFS +=		-DACE_BUILD_SVC_DLL

include $(ROOTDIR)/config/depend.mk

# dependencies
include $(ROOTDIR)/libutp/src/export.mk

include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
