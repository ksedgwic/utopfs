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

# Test basic link operations.

class Test_fs_link_01:

  def setup_class(self):
    self.bspath = "fs_link_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "", "", CONFIG.FSARGS)

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_link(self):

    # Create a directory.
    self.fs.fs_mkdir("/foo", 0555)

    # Create a file.
    self.fs.fs_mknod("/foo/bar", 0666, 0)

    # Open the file.
    self.fs.fs_open("/foo/bar", O_RDWR)

    # Write some bytes into the file.
    self.fs.fs_write("/foo/bar", buffer("testdata"))

    # Check clash w/ existing file.
    try:
      self.fs.fs_link("/blahblah", "/foo/bar");
      assert False
    except OSError, ex:
      assert ex.errno == EEXIST
    
    # Check clash w/ existing directory.
    try:
      self.fs.fs_link("/blahblah", "/foo");
      assert False
    except OSError, ex:
      assert ex.errno == EEXIST
    
    # Create a link to the file.
    self.fs.fs_link("/foo/bar", "/foobar")

    # Stat the file.
    st = self.fs.fs_getattr("/foo/bar")
    assert S_ISREG(st[ST_MODE])

    # Stat the link.
    st = self.fs.fs_getattr("/foobar")
    assert S_ISREG(st[ST_MODE])

    # Can't remove link w/ rmdir
    try:
      self.fs.fs_rmdir("/foobar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOTDIR
    
    # Can remove the original file
    self.fs.fs_unlink("/foo/bar");

    # Now the file shouldn't stat
    try:
      st = self.fs.fs_getattr("/foo/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # But the linked file can still stat.
    st = self.fs.fs_getattr("/foobar")
    assert S_ISREG(st[ST_MODE])

    # Remove the link w/ unlink.
    self.fs.fs_unlink("/foobar");

    # Now the link shouldn't stat
    try:
      st = self.fs.fs_getattr("/foobar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT
