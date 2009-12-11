import sys
import random
import py


from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

# This test ensures that sparse file support works.
#
# Each level of indirection multiples the supported size
# by BLKSZ / sizeof(BlockRef) = 8K / 32 = 256
#
class Test_fs_sparse_01:

  def setup_class(self):
    self.bspath = "fs_sparse_01.bs"

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_can_write_sparse(self):

    print "Remove any prexisting blockstore."
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

    print "Create the filesystem"
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    print "Create a new file."
    self.fs.fs_mknod("/sparse", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    print "Create a 18000 byte string."
    jnkstr = ""
    for i in range(0, 2000):
      jnkstr += "%08d\n" % (i)
    assert jnkstr[0:8] == "00000000"
    assert jnkstr[18000-9:18000-1] == "00001999"

    print "Write the buffer to the file."
    rv = self.fs.fs_write("/sparse", buffer(jnkstr))
    assert rv == 18000

    print "Stat should show the size."
    st = self.fs.fs_getattr("/sparse")
    assert st.st_size == 18000

    print "Check the number of file blocks."
    # inode + 2 data blocks * 8192 / 512
    nblocks = st.st_blocks
    assert nblocks == (1 + 2) * 8192 / 512

    print "---------------- indirect blocks ----------------"

    print "Write the buffer at 1Mbyte offset."
    rv = self.fs.fs_write("/sparse", buffer(jnkstr), 1024 * 1024)

    print "Size should include the whole space."
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + (1024 * 1024)

    print "But the number of blocks is much less ..."
    # inode + 2 data blocks + indirect + 3 data blocks
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 + 1 + 3) * 8192 / 512

    print "We should be able to read the data."
    buf = self.fs.fs_read("/sparse", 18000, 1024 * 1024)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    print "Reading somewhere in the \"hole\" should see zeros."
    buf = self.fs.fs_read("/sparse", 18000, 512 * 1024)
    assert str(buf)[0] == '\0'
    assert str(buf)[18000-1] == '\0'

    print "And the read shouldn't change the size or block count."
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + (1024 * 1024)
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 + 1 + 3) * 8192 / 512

    print "---------------- double indirect ----------------"

    print "Write the buffer at 100 Mbyte offset."
    off = 100 * 1024 * 1024
    rv = self.fs.fs_write("/sparse", buffer(jnkstr), off)

    print "Size should include the whole space."
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + off

    print "But the number of blocks is much less ..."
    # inode + 2 data blocks
    #       + indirect + 3 data blocks
    #       + doubleind + 3 data blocks
    #
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 + 1 + 3 + 1 + 1 + 3) * 8192 / 512

    print "We should be able to read the data."
    buf = self.fs.fs_read("/sparse", 18000, off)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    print "Reading somewhere in the \"hole\" should see zeros."
    buf = self.fs.fs_read("/sparse", 18000, off/2)
    assert str(buf)[0] == '\0'
    assert str(buf)[18000-1] == '\0'

    print "And the read shouldn't change the size or block count."
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + off
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 + 1 + 3 + 1 + 1 + 3) * 8192 / 512

    print "Refresh the blocks associated w/ the filesystem"
    nb = self.fs.fs_refresh();
    assert nb == 13
    
    print "---------------- tripe indirect ----------------"

    print "Write the buffer at 25 Gbyte offset."
    off = 25 * 1024 * 1024 * 1024
    rv = self.fs.fs_write("/sparse", buffer(jnkstr), off)

    print "Size should include the whole space."
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + off

    print "But the number of blocks is much less ..."
    # inode + 2 data blocks
    #       + indirect + 3 data blocks
    #       + doubleind + inddirect + 3 data blocks
    #       + tripleind + double + indirect + 3 data blocks
    #
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 +
                       1 + 3 +
                       1 + 1 + 3 +
                       1 + 1 + 1 + 3) * 8192 / 512

    print "We should be able to read the data."
    buf = self.fs.fs_read("/sparse", 18000, off)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    print "Reading somewhere in the \"hole\" should see zeros."
    buf = self.fs.fs_read("/sparse", 18000, off/2)
    assert str(buf)[0] == '\0'
    assert str(buf)[18000-1] == '\0'

    print "And the read shouldn't change the size or block count."
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + off
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 +
                       1 + 3 +
                       1 + 1 + 3 +
                       1 + 1 + 1 + 3) * 8192 / 512

    print "Refresh the blocks associated w/ the filesystem"
    nb = self.fs.fs_refresh();
    assert nb == 19
    
    print "---------------- remount ----------------"

    print "Now we unmount the filesystem."
    self.fs.fs_umount()
    self.bs.bs_close()


    print "Now mount it again."
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, "rootbs", bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    print "Size should include the whole space."
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + off

    print "But the number of blocks is much less ..."
    # inode + 2 data blocks + indirect + 3 data blocks
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 +
                       1 + 3 +
                       1 + 1 + 3 +
                       1 + 1 + 1 + 3) * 8192 / 512

    print "We should be able to read the data."
    self.fs.fs_open("/sparse", O_RDONLY)
    buf = self.fs.fs_read("/sparse", 18000, off)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    print "Reading somewhere in the \"hole\" should see zeros."
    buf = self.fs.fs_read("/sparse", 18000, off/2)
    assert str(buf)[0] == '\0'
    assert str(buf)[18000-1] == '\0'

    print "And the read shouldn't change the size or block count."
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + off
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 +
                       1 + 3 +
                       1 + 1 + 3 +
                       1 + 1 + 1 + 3) * 8192 / 512

    print "Refresh the blocks associated w/ the filesystem"
    nb = self.fs.fs_refresh();
    assert nb == 19
    
    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs.bs_close()
    self.bs = None
    self.fs = None

