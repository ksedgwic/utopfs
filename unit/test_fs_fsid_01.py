import sys
import random
import py


from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

class Test_fs_fsid_01:

  def setup_class(self):
    self.bspath = "fs_fsid_01.bs"

    # Remove any prexisting blockstore.
    CONFIG.remove_bs(self.bspath)

  def teardown_class(self):
    CONFIG.remove_bs(self.bspath)

  def test_separate_fsid(self):

    # Create the filesystem
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, CONFIG.BSSIZE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "first", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Make a file.
    self.fs.fs_mknod("/first", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()

    # Now mount it again.
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)

    py.test.raises(utp.NotFoundError,
                   utp.FileSystem.mount, CONFIG.FSTYPE, self.bs,
                   "second", "", CONFIG.FSARGS)

    # Now mount it again.
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "first", "", CONFIG.FSARGS)

    # File should be there.
    st = self.fs.fs_getattr("/first")

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None
