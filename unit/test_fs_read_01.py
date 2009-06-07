import sys
import random
import py

from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

# Make sure we can read a block of data larger then we wrote.

class Test_fs_read_01:

  def setup_class(self):
    self.bspath = "fs_read_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "", "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_read(self):

    # Create a file.
    self.fs.fs_mknod("/foo", 0666, 0)

    # Write some data into the file.
    self.fs.fs_write("/foo", buffer("testdata"))

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 8

    # Now try and read 4096 bytes.
    buf = self.fs.fs_read("/foo", 4096)
    assert str(buf) == "testdata"
    
    # Now try and read 4097 bytes.
    buf = self.fs.fs_read("/foo", 4097)
    assert str(buf) == "testdata"
    
    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "", "")

    # We should be able to stat the same file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 8

    # We should be able to read the data.
    self.fs.fs_open("/foo", O_RDONLY)
    buf = self.fs.fs_read("/foo", 4096)
    assert str(buf) == "testdata"

    # We should be able to read the data.
    self.fs.fs_open("/foo", O_RDONLY)
    buf = self.fs.fs_read("/foo", 4097)
    assert str(buf) == "testdata"
