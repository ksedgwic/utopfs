import sys
import random
import py

from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

# This test checks that data written to a file is
# persisted.

class Test_fs_write_01:

  def setup_class(self):
    self.bspath = "fs_write_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_write(self):

    # Create a file.
    self.fs.fs_mknod("/foo", 0666, 0)

    # Write some data into the file.
    self.fs.fs_write("/foo", buffer("testdata"))

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 8
    
    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "")

    # We should be able to stat the same file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 8

    # We should be able to read the data.
    buf = self.fs.fs_read("/foo")
    assert str(buf) == "testdata"
