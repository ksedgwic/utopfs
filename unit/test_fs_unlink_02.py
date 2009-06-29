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

# There were problems w/ removing the only member of a directory.

class Test_fs_unlink_02:

  def setup_class(self):
    self.bspath = "fs_unlink_02.bs"

    # Remove any prexisting blockstore.
    CONFIG.remove_bs(self.bspath)  

  def teardown_class(self):
    CONFIG.remove_bs(self.bspath) 

  def test_unlink(self):

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a file
    self.fs.fs_mknod("/bar", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");

    # Should be able to unlink the file.
    self.fs.fs_unlink("/bar");

    # Now the file shouldn't stat
    try:
      st = self.fs.fs_getattr("/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # Now we unmount the filesystem.
    self.fs.fs_umount()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Now the file shouldn't stat
    try:
      st = self.fs.fs_getattr("/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None
