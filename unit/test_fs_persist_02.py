import sys
import random
import py

from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

class Test_fs_persist_02:

  def setup_class(self):
    self.bspath = "fs_persist_02.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_persistence(self):

    # Create a diretory.
    self.fs.fs_mkdir("/foo", 0755)

    # Create a file in the directory.
    self.fs.fs_open("/foo/bar", O_CREAT)

    # Now we should be able to stat the directory.
    st = self.fs.fs_getattr("/foo");
    assert S_ISDIR(st[ST_MODE])
    
    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo/bar");
    assert S_ISREG(st[ST_MODE])
    
    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "")

    # Now we should be able to stat the directory.
    st = self.fs.fs_getattr("/foo");
    assert S_ISDIR(st[ST_MODE])
    
    # We should be able to stat the same file.
    st = self.fs.fs_getattr("/foo/bar");
    assert S_ISREG(st[ST_MODE])
