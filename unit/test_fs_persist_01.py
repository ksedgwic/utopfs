import sys
import random
import py

from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

class TestBlockStorefsmkfs:

  def setup_class(self):
    self.bspath = "fs_persist_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_persistence(self):
    # Create a file.
    self.fs.fs_open("/foo", O_CREAT)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
    
    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "")

    # We should be able to stat the same file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
