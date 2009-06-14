import sys
import random
import py
import shutil

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
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "", "", CONFIG.FSARGS)

  def teardown_class(self):
    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    utp.FileSystem.logoff()
    shutil.rmtree(self.bspath,True) 

  def test_rename(self):

    # Create a directory.
    self.fs.fs_mkdir("/foo", 0555)

    # Create a file.
    self.fs.fs_mknod("/foo/bar", 0666, 0)

    # Open the file.
    self.fs.fs_open("/foo/bar", O_RDWR)

    # Write some bytes into the file.
    self.fs.fs_write("/foo/bar", buffer("testdata"))

    # Should be able to rename the file.
    self.fs.fs_rename("/foo/bar", "/foo/blat")

    # Now the old name shouldn't exist.
    try:
      st = self.fs.fs_getattr("/foo/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # And the new one should.
    st = self.fs.fs_getattr("/foo/blat");

    # Should be able to rename a directory.
    self.fs.fs_rename("/foo", "/bar")
    
    # The file should be accessable in the new directory.
    st = self.fs.fs_getattr("/bar/blat");

    # Should be able to rename into a directory.
    self.fs.fs_mkdir("/adir", 0555)
    self.fs.fs_rename("/bar/blat", "/adir/blat")
    st = self.fs.fs_getattr("/adir/blat");
