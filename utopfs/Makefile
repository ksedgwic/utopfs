ROOTDIR = ..

include $(ROOTDIR)/config/define.mk

PRGSRC += 	\
			utopfs.cpp \
			$(NULL)

PRGEXE = 	utopfs


SCRIPTS =	\
			$(NULL)

CFG =		\
			svc.conf \
			$(NULL)

include $(ROOTDIR)/config/depend.mk

# Dependencies
include $(ROOTDIR)/libutp/src/export.mk

# External Package Dependencies
include $(ROOTDIR)/config/extdep/fuse.mk
include $(ROOTDIR)/config/extdep/ACE.mk

# Module Dependencies
include $(ROOTDIR)/modules/DefaultLogger/export.mk
include $(ROOTDIR)/modules/FSBlockStore/export.mk
include $(ROOTDIR)/modules/UTFileSystem/export.mk
