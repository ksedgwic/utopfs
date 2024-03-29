import sys
import random
import py


from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

# Make sure we can read a block of data larger then we wrote.

class Test_fs_read_01:

  def setup_class(self):
    self.bspath = "fs_read_01.bs"

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_read(self):

    # Remove any prexisting blockstore.
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

    # Create the filesystem
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a file.
    self.fs.fs_mknod("/foo", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Write some data into the file.
    self.fs.fs_write("/foo", buffer("testdata"))

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 8

    # Now try and read 4096 bytes.
    buf = self.fs.fs_read("/foo", 4096)
    assert str(buf) == "testdata"
    
    # Now try and read 4097 bytes.
    buf = self.fs.fs_read("/foo", 4097)
    assert str(buf) == "testdata"
    
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

    # We should be able to stat the same file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 8

    # We should be able to read the data.
    self.fs.fs_open("/foo", O_RDONLY)
    buf = self.fs.fs_read("/foo", 4096)
    assert str(buf) == "testdata"

    # We should be able to read the data.
    self.fs.fs_open("/foo", O_RDONLY)
    buf = self.fs.fs_read("/foo", 4097)
    assert str(buf) == "testdata"

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs.bs_close()
    self.bs = None
    self.fs = None
