import sys
import random
import py

from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

class Test_fs_unlink_01:

  def setup_class(self):
    self.bspath = "fs_unlink_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "", "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_unlink(self):

    # Create a file
    self.fs.fs_mknod("/bar", 0666, 0)

    # Create a directory
    self.fs.fs_mkdir("/foo", 0555)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");

    # Now we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");

    # Shouldn't be able to unlink the directory.
    try:
      self.fs.fs_unlink("/foo");
      assert False
    except OSError, ex:
      assert ex.errno == 21

    # Should be able to unlink the file.
    self.fs.fs_unlink("/bar");

    # Now the file shouldn't stat
    try:
      st = self.fs.fs_getattr("/bar");
      assert False
    except OSError, ex:
      assert ex.errno == 2

    # But we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");

    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "", "")

    # Now the file shouldn't stat
    try:
      st = self.fs.fs_getattr("/bar");
      assert False
    except OSError, ex:
      assert ex.errno == 2

    # But we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");

