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

class Test_fs_unlink_01:

  def setup_class(self):
    self.bspath = "fs_unlink_01.bs"

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_unlink(self):

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

    print "Create a file"
    self.fs.fs_mknod("/bar", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    print "Create a directory"
    self.fs.fs_mkdir("/foo", 0555, CONFIG.UNAME, CONFIG.GNAME)

    print "Now we should be able to stat the file."
    st = self.fs.fs_getattr("/bar");

    print "Now we should be able to stat the dir."
    st = self.fs.fs_getattr("/foo");

    print "Shouldn't be able to unlink the directory."
    try:
      self.fs.fs_unlink("/foo");
      assert False
    except OSError, ex:
      assert ex.errno == EISDIR

      print "Should be able to unlink the file."
    self.fs.fs_unlink("/bar");

    print "Now the file shouldn't stat"
    try:
      st = self.fs.fs_getattr("/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

      print "But we should be able to stat the dir."
    st = self.fs.fs_getattr("/foo");

    print "Now we unmount the filesystem."
    self.fs.fs_umount()
    self.bs.bs_close()

    print "Now mount it again."
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, "rootbs", bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    print "Now the file shouldn't stat"
    try:
      st = self.fs.fs_getattr("/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

      print "But we should be able to stat the dir."
    st = self.fs.fs_getattr("/foo");

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs.bs_close()
    self.bs = None
    self.fs = None
