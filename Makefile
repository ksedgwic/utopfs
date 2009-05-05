ROOTDIR = .

include $(ROOTDIR)/config/define.mk

SUBDIRS =	\
			libutp \
			utopfs \
			modules \
			spec \
			$(NULL)

include $(ROOTDIR)/config/depend.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
