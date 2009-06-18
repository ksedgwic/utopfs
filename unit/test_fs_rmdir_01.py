import sys
import random
import py
import shutil

from os import *
from stat import *
from errno import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

class Test_fs_rmdir_01:

  def setup_class(self):
    self.bspath = "fs_rmdir_01.bs"

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_rmdir(self):

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "", "", CONFIG.FSARGS)

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
    self.fs.fs_umount()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Now the directory shouldn't stat
    try:
      st = self.fs.fs_getattr("/foo");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # But we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None

  def test_rmdir_nonempty(self):

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "", "", CONFIG.FSARGS)

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

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None