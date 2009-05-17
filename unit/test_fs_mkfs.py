import sys
import random
import py

import utp
import utp.FileSystem

class TestBlockStore:

  def setup_class(self):
    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
  def teardown_class(self):
    pass

  def test_mkfs(self):
    self.fs.fs_mkfs("MKFS.BLK");
