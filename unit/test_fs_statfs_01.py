import sys
import random
import py


from os import *
from stat import *
from errno import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

class Test_fs_statfs_01:

  def setup_class(self):
    self.bspath = "fs_statfs_01.bs"

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_statfs(self):

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

    BLKSZ = 4 * 1024
    BLKSUSED = 2

    stvfs = self.fs.fs_statfs()
    assert stvfs.f_bsize == BLKSZ
    assert stvfs.f_blocks == CONFIG.BSSIZE / BLKSZ
    assert stvfs.f_bfree == (CONFIG.BSSIZE / BLKSZ) - BLKSUSED
    assert stvfs.f_bavail == (CONFIG.BSSIZE / BLKSZ) - BLKSUSED

    # Now we unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()

    # Now mount it again.
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, "rootbs", bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    stvfs = self.fs.fs_statfs()
    assert stvfs.f_bsize == BLKSZ
    assert stvfs.f_blocks == CONFIG.BSSIZE / BLKSZ
    assert stvfs.f_bfree == (CONFIG.BSSIZE / BLKSZ) - BLKSUSED
    assert stvfs.f_bavail == (CONFIG.BSSIZE / BLKSZ) - BLKSUSED

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs.bs_close()
    self.bs = None
    self.fs = None
