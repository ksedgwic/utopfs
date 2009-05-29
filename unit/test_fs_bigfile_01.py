import sys
import random
import py
from os import *
from stat import *

import shutil
import utp
import utp.FileSystem
import utp.PyDirEntryFunc

# This test ensures that multiple block files are working correctly.

class Test_fs_bigfile_01:

  def setup_class(self):
    self.bspath = "fs_bigfile_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_can_write_bigfile(self):

    # Create a new file.
    self.fs.fs_mknod("/bigfile", 0666, 0)

    # Create a 18000 byte string"
    jnkstr = ""
    for i in range(0, 2000):
      jnkstr += "%08d\n" % (i)

    # Write the buffer to the file.
    rv = self.fs.fs_write("/bigfile", buffer(jnkstr))
    assert rv == len(jnkstr)
