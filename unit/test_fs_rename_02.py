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

# Make sure rename can replace existing files.

class Test_fs_rename_02:

  def setup_class(self):
    self.bspath = "fs_rename_02.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "", "", CONFIG.FSARGS)

  def teardown_class(self):
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
    assert st.st_size == 8

    # Should be able to replace a file w/ rename
    self.fs.fs_mknod("/foo/new", 0666, 0)

    # Open the file.
    self.fs.fs_open("/foo/new", O_RDWR)

    # Write some bytes into the file.
    self.fs.fs_write("/foo/new", buffer("newdata"))

    # Rename the new over the old.
    self.fs.fs_rename("/foo/new", "/foo/blat")

    # Now the old name shouldn't exist.
    try:
      st = self.fs.fs_getattr("/foo/new");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # And the new one should.
    st = self.fs.fs_getattr("/foo/blat");
    assert st.st_size == 7

    
