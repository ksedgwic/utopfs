ROOTDIR = .

include $(ROOTDIR)/config/define.mk

SUBDIRS =	\
			utopfs \
			spec \
			$(NULL)

include $(ROOTDIR)/config/depend.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
