ROOTDIR = ..

include $(ROOTDIR)/config/define.mk

PRGSRC += 	\
			Controller.cpp \
			ControlService.cpp \
			fuselog.cpp \
			StatsLogger.cpp \
			utopfs.cpp \
			$(NULL)

PRGEXE = 	utopfs


SCRIPTS =	\
			runmkfs \
			runmount \
			runmkfs.s3 \
			runmount.s3 \
			$(NULL)

CFG =		\
			utopfs.conf \
			utopfs.cfg \
			utopfs0.cfg \
			utopfs1.cfg \
			utopfs2.cfg \
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
include $(ROOTDIR)/modules/BDBBlockStore/export.mk
include $(ROOTDIR)/modules/S3BlockStore/export.mk
include $(ROOTDIR)/modules/VBlockStore/export.mk
include $(ROOTDIR)/modules/UTFileSystem/export.mk
