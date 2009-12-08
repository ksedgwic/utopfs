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

# Test basic rename operations.

class Test_fs_rename_01:

  def setup_class(self):
    self.bspath = "fs_rename_01.bs"

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

  def teardown_class(self):
    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    olvl = utp.FileSystem.loglevel(-1)
    self.bs.bs_close()
    self.bs = None
    self.fs = None
    utp.FileSystem.loglevel(olvl)

    CONFIG.remove_bs(self.bspath)

  def test_rename(self):

    print "Create a directory."
    self.fs.fs_mkdir("/foo", 0555, CONFIG.UNAME, CONFIG.GNAME)

    print "Create a file."
    self.fs.fs_mknod("/foo/bar", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    print "Open the file."
    self.fs.fs_open("/foo/bar", O_RDWR)

    print "Write some bytes into the file."
    self.fs.fs_write("/foo/bar", buffer("testdata"))

    print "Should be able to rename the file."
    self.fs.fs_rename("/foo/bar", "/foo/blat")

    print "Now the old name shouldn't exist."
    try:
      st = self.fs.fs_getattr("/foo/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    print "And the new one should."
    st = self.fs.fs_getattr("/foo/blat");

    print "Should be able to rename a directory."
    self.fs.fs_rename("/foo", "/bar")
    
    print "The file should be accessable in the new directory."
    st = self.fs.fs_getattr("/bar/blat");

    print "Should be able to rename into a directory."
    self.fs.fs_mkdir("/adir", 0555, CONFIG.UNAME, CONFIG.GNAME)
    self.fs.fs_rename("/bar/blat", "/adir/blat")
    st = self.fs.fs_getattr("/adir/blat");
