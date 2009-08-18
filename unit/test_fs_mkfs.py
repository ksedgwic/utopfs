import sys
import random
import py


from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

class Test_fs_mkfs:

  def setup_class(self):
    self.bspath = "fs_mkfs.bs"

    # Remove any prexisting blockstore.
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

  def teardown_class(self):

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs.bs_close()
    olvl = utp.FileSystem.loglevel(-1)
    self.bs = None
    self.fs = None
    utp.FileSystem.loglevel(olvl)
    
    CONFIG.remove_bs(self.bspath)

  def test_hasno_foodir(self):
    # The filesystem should not have a "foo" directory.
    py.test.raises(OSError, "self.fs.fs_getattr('/foo')")

  def test_has_dotdir(self):
    # The filesystem should have a .utopfs directory.
    statbuf = self.fs.fs_getattr("/.utopfs")
    assert S_ISDIR(statbuf[ST_MODE])

    # The filesystem should have a version file.
    statbuf = self.fs.fs_getattr("/.utopfs/version")
    assert S_ISREG(statbuf[ST_MODE])

    # The version file should contain a version string.
    self.fs.fs_open("/.utopfs/version", O_RDONLY)
    data = self.fs.fs_read("/.utopfs/version", 100, 0)
    assert str(data).find("utopfs version") == 0

  def test_can_create_file(self):
    # We should be able to create a file.
    self.fs.fs_mknod("/foo", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Now we should be able to stat the file.
    print self.fs.fs_getattr("/foo");
