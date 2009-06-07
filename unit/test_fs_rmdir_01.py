import sys
import random
import py

from os import *
from stat import *
from errno import *

import shutil
import utp
import utp.FileSystem

class Test_fs_rmdir_01:

  def setup_class(self):
    self.bspath = "fs_rmdir_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "", "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_rmdir(self):

    # Create a file.
    self.fs.fs_mknod("/bar", 0666, 0)

    # Create a directory.
    self.fs.fs_mkdir("/foo", 0555)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");

    # Now we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");

    # Shouldn't be able to rmdir the file.
    try:
      self.fs.fs_rmdir("/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOTDIR

    # Should be able to rmdir the directory.
    self.fs.fs_rmdir("/foo");

    # Now the directory shouldn't stat
    try:
      st = self.fs.fs_getattr("/foo");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # But we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");

    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "", "")

    # Now the directory shouldn't stat
    try:
      st = self.fs.fs_getattr("/foo");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # But we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");

  def test_rmdir_nonempty(self):

    # Create a directory.
    self.fs.fs_mkdir("/top", 0555)

    # Create a file.
    self.fs.fs_mknod("/top/inside", 0666, 0)

    # Shouldn't be able to rmdir the directory.
    try:
      self.fs.fs_rmdir("/top");
      assert False
    except OSError, ex:
      assert ex.errno == ENOTEMPTY
