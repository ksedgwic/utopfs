import sys
import random
import py
from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

class Test_fs_fsid_01:

  def setup_class(self):
    self.bspath = "fs_fsid_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_separate_fsid(self):
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "first", "")

    # Make a file.
    self.fs.fs_mknod("/first", 0666, 0)

    # Unmount the filesystem.
    self.fs.fs_unmount()

    # Mount the filesystem w/ the wrong fsid.
    py.test.raises(utp.NotFoundError,
                   "self.fs.fs_mount(self.bspath, \"second\", \"\")")

    # Mount the filesystem w/ the correct fsid.
    self.fs.fs_mount(self.bspath, "first", "")

    # File should be there.
    st = self.fs.fs_getattr("/first")
