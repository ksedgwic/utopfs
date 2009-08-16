import sys
import random
import py


from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

# This test ensures that multiple block files are working correctly.

class Test_fs_bigfile_01:

  def setup_class(self):
    self.bspath = "fs_bigfile_01.bs"

  def teardown_class(self):
    CONFIG.remove_bs(self.bspath)

  def test_can_write_bigfile(self):

    # Remove any prexisting blockstore.
    CONFIG.remove_bs(self.bspath)

    # Create the filesystem
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a new file.
    self.fs.fs_mknod("/bigfile", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Create a 18000 byte string"
    jnkstr = ""
    for i in range(0, 2000):
      jnkstr += "%08d\n" % (i)
    assert jnkstr[0:8] == "00000000"
    assert jnkstr[18000-9:18000-1] == "00001999"

    # Write the buffer to the file.
    rv = self.fs.fs_write("/bigfile", buffer(jnkstr))
    assert rv == 18000

    # Stat should show the size.
    st = self.fs.fs_getattr("/bigfile");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 18000
    nblocks = st.st_blocks
    assert nblocks == (1 + 2) * 8192 / 512

    # We should be able to read the data.
    self.fs.fs_open("/bigfile", O_RDONLY)
    buf = self.fs.fs_read("/bigfile", 18000)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    # Refresh the blocks associated w/ the filesystem
    nb = self.fs.fs_refresh();
    assert nb == 4
    
    # Now we unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()

    # Now mount it again.
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Stat should show the size.
    st = self.fs.fs_getattr("/bigfile");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 18000
    nblocks = st.st_blocks
    assert nblocks == (1 + 2) * 8192 / 512

    # We should be able to read the data.
    self.fs.fs_open("/bigfile", O_RDONLY)
    buf = self.fs.fs_read("/bigfile", 18000)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    # Refresh the blocks associated w/ the filesystem
    nb = self.fs.fs_refresh();
    assert nb == 4
    
    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs.bs_close()
    self.bs = None
    self.fs = None
