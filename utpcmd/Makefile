ROOTDIR = ..

include $(ROOTDIR)/config/define.mk

PRGSRC += 	\
			utp.cpp \
			$(NULL)

PRGEXE = 	utp


SCRIPTS =	\
			runutp \
			$(NULL)

CFG =		\
			utp.conf \
			$(NULL)

include $(ROOTDIR)/config/depend.mk

# Dependencies
include $(ROOTDIR)/libutp/src/export.mk

# External Package Dependencies
include $(ROOTDIR)/config/extdep/ACE.mk

# Module Dependencies
include $(ROOTDIR)/modules/DefaultLogger/export.mk
include $(ROOTDIR)/modules/FSBlockStore/export.mk
include $(ROOTDIR)/modules/BDBBlockStore/export.mk
include $(ROOTDIR)/modules/S3BlockStore/export.mk
include $(ROOTDIR)/modules/VBlockStore/export.mk
include $(ROOTDIR)/modules/UTFileSystem/export.mk
