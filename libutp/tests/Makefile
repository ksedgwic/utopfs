ROOTDIR = ../..

include $(ROOTDIR)/config/define.mk

# Add test programs (one mainline per source file) to TSTSRC

TSTSRC += 	\
			blockcipher.cpp \
			scoped.cpp \
			streamcipher.cpp \
			$(NULL)

include $(ROOTDIR)/config/depend.mk

# Dependencies
include $(ROOTDIR)/libutp/src/export.mk

# External Package Dependencies
include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
