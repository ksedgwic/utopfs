import sys
import random
import py

from os import *
from stat import *
from errno import *

import shutil
import utp
import utp.FileSystem

# Test basic truncate operations.

class Test_fs_truncate_01:

  def setup_class(self):
    self.bspath = "fs_truncate_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "", "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_trucate(self):

    # Create a directory.
    self.fs.fs_mkdir("/foo", 0555)

    # Create a file.
    self.fs.fs_mknod("/foo/bar", 0666, 0)

    # Open the file.
    self.fs.fs_open("/foo/bar", O_RDWR)

    # Write some bytes into the file.
    self.fs.fs_write("/foo/bar", buffer("testdata"))

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo/bar");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 8

    # Truncate the file to 4 bytes.
    self.fs.fs_truncate("/foo/bar", 4)

    # Now try and read the bytes.
    buf = self.fs.fs_read("/foo/bar", 4096)
    assert str(buf) == "test"
        
    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo/bar");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 4

    # Truncate the file to 0 bytes.
    self.fs.fs_truncate("/foo/bar", 0)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo/bar");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 0
