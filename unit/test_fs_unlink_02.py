import sys
import random
import py

from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

# There were problems w/ removing the only member of a directory.

class Test_fs_unlink_02:

  def setup_class(self):
    self.bspath = "fs_unlink_02.bs"

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

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");

    # Should be able to unlink the file.
    self.fs.fs_unlink("/bar");

    # Now the file shouldn't stat
    try:
      st = self.fs.fs_getattr("/bar");
      assert False
    except OSError, ex:
      assert ex.errno == 2

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
