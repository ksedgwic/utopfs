ROOTDIR = ..

include $(ROOTDIR)/config/define.mk

SUBDIRS =	\
			DefaultLogger \
			FSBlockStore \
			VBlockStore \
			UTFileSystem \
			$(NULL)

ifneq (WIN32,$(SYSNAME))
SUBDIRS +=	\
			BDBBlockStore \
			S3BlockStore \
			$(NULL)
endif

include $(ROOTDIR)/config/depend.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
