# Copyright (c) 2007 Bonsai Software, Inc.  All rights reserved.

ROOTDIR =	..

include $(ROOTDIR)/config/define.mk

SCRIPTS =	\
			test1 \
			test_fileblockstore.py \
			$(NULL)

CFG =		\
			svc.conf \
			$(NULL)

all::
			(cd $(WORKCFGDIR) && py.test test_fileblockstore.py)


include $(ROOTDIR)/config/depend.mk

# Dependencies
include $(ROOTDIR)/libutp/pymod/export.mk
include $(ROOTDIR)/libutp/src/export.mk

# External Package Dependencies
include $(ROOTDIR)/config/extdep/ACE.mk
include $(ROOTDIR)/config/extdep/python.mk

# Module Dependencies
include $(ROOTDIR)/modules/DefaultLogger/export.mk
include $(ROOTDIR)/modules/FSBlockStore/export.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
