import sys
import random
import py

from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

class Test_fs_chmod_02:

  def setup_class(self):
    self.bspath = "fs_chmod_02.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "", "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_chmod(self):

    # Open up our umask for this experiment
    umask(0000)

    # Create a file
    self.fs.fs_mknod("/bar", 0666, 0)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_MODE] & 0777 == 0666

    # Now we chmod the file.
    self.fs.fs_chmod("/bar", 0640)
    
    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_MODE] & 0777 == 0640

    # Create a directory
    self.fs.fs_mkdir("/foo", 0555)

    # Now we chmod the directory.
    self.fs.fs_chmod("/foo", 0510)
    
    # Now we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");
    assert st[ST_MODE] & 0777 == 0510

    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "", "")

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_MODE] & 0777 == 0640

    # Now we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");
    assert st[ST_MODE] & 0777 == 0510
