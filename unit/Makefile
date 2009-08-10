ROOTDIR =	..

include $(ROOTDIR)/config/define.mk

PYTESTS =	\
			test_bs_unopened.py \
			test_bs_basic_01.py \
			test_bs_refresh_01.py \
			test_bs_size_01.py \
			test_bs_size_02.py \
			test_bs_size_03.py \
			test_bs_head_01.py \
			test_bs_head_02.py \
			test_bs_head_03.py \
			test_fs_mkfs.py \
			test_fs_persist_01.py \
			test_fs_persist_02.py \
			test_fs_getattr_01.py \
			test_fs_getattr_02.py \
			test_fs_readdir_01.py \
			test_fs_readdir_02.py \
			test_fs_write_01.py \
			test_fs_bigfile_01.py \
			test_fs_sparse_01.py \
			test_fs_fsid_01.py \
			test_fs_chmod_01.py \
			test_fs_unlink_01.py \
			test_fs_unlink_02.py \
			test_fs_read_01.py \
			test_fs_rmdir_01.py \
			test_fs_symlink_01.py \
			test_fs_symlink_02.py \
			test_fs_truncate_01.py \
			test_fs_truncate_02.py \
			test_fs_truncate_03.py \
			test_fs_rename_01.py \
			test_fs_rename_02.py \
			test_fs_link_01.py \
			test_fs_statfs_01.py \
			test_fs_nospace_01.py \
			test_fs_bigdir_01.py \
			test_fs_doubleslash_01.py \
			test_fs_syncmiddle_01.py \
			$(NULL)

SCRIPTS =	\
			$(PYTESTS) \
			test1 \
			test2 \
			refresh \
			s3test \
			rmbuck \
			$(NULL)

CFG =		\
			CONFIG.py \
			lenhack.py \
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
include $(ROOTDIR)/modules/BDBBlockStore/export.mk
include $(ROOTDIR)/modules/S3BlockStore/export.mk
include $(ROOTDIR)/modules/UTFileSystem/export.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
