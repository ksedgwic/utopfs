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

# Test basic symlink operations.

class Test_fs_symlink_01:

  def setup_class(self):
    self.bspath = "fs_symlink_01.bs"

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_symlink(self):

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a directory.
    self.fs.fs_mkdir("/foo", 0555, CONFIG.UNAME, CONFIG.GNAME)

    # Create a file.
    self.fs.fs_mknod("/foo/bar", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Open the file.
    self.fs.fs_open("/foo/bar", O_RDWR)

    # Write some bytes into the file.
    self.fs.fs_write("/foo/bar", buffer("testdata"))

    # Check clash w/ existing file.
    try:
      self.fs.fs_symlink("/blahblah", "/foo/bar");
      assert False
    except OSError, ex:
      assert ex.errno == EEXIST
    
    # Check clash w/ existing directory.
    try:
      self.fs.fs_symlink("/blahblah", "/foo");
      assert False
    except OSError, ex:
      assert ex.errno == EEXIST
    
    # Create a symbolic link to the file.
    self.fs.fs_symlink("/foo/bar", "/foobar")

    # Stat the file.
    st = self.fs.fs_getattr("/foo/bar")
    assert S_ISREG(st[ST_MODE])

    # Stat the link.
    st = self.fs.fs_getattr("/foobar")
    assert S_ISLNK(st[ST_MODE])

    # Read the link w/ readlink
    symlink = self.fs.fs_readlink("/foobar")
    assert symlink == "/foo/bar"

    # Files make readlink sad.
    try:
      lval = self.fs.fs_readlink("/foo/bar")
      assert False
    except OSError, ex:
      assert ex.errno == EINVAL
    
    # Dirs make readlink sad.
    try:
      lval = self.fs.fs_readlink("/foo")
      assert False
    except OSError, ex:
      assert ex.errno == EINVAL
    
    # Can't remove symlink w/ rmdir
    try:
      self.fs.fs_rmdir("/foobar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOTDIR
    
    # Can remove the file from under the symlink.
    self.fs.fs_unlink("/foo/bar");

    # Now the file shouldn't stat
    try:
      st = self.fs.fs_getattr("/foo/bar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # But the link can still stat.
    st = self.fs.fs_getattr("/foobar")
    assert S_ISLNK(st[ST_MODE])

    # And the link can still be read.
    symlink = self.fs.fs_readlink("/foobar")
    assert symlink == "/foo/bar"

    # Remove the symlink w/ unlink.
    self.fs.fs_unlink("/foobar");

    # Now the link shouldn't stat
    try:
      st = self.fs.fs_getattr("/foobar");
      assert False
    except OSError, ex:
      assert ex.errno == ENOENT

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None
