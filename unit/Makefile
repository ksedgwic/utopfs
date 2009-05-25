ROOTDIR =	..

include $(ROOTDIR)/config/define.mk

PYTESTS =	\
			test_fs_mkfs.py \
			test_bs_unopened.py \
			test_fileblockstore.py \
			test_fs_persist_01.py \
			$(NULL)

SCRIPTS =	\
			$(PYTESTS) \
			test1 \
			test2 \
			$(NULL)

CFG =		\
			svc.conf \
			$(NULL)

include $(ROOTDIR)/config/depend.mk

test::
	(cd $(WORKCFGDIR) && . ./env.sh && py.test $(PYTESTS))

# Dependencies
include $(ROOTDIR)/libutp/pymod/export.mk
include $(ROOTDIR)/libutp/src/export.mk

# External Package Dependencies
include $(ROOTDIR)/config/extdep/ACE.mk
include $(ROOTDIR)/config/extdep/python.mk

# Module Dependencies
include $(ROOTDIR)/modules/DefaultLogger/export.mk
include $(ROOTDIR)/modules/FSBlockStore/export.mk
include $(ROOTDIR)/modules/UTFileSystem/export.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
