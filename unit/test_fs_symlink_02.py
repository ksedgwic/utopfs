import sys
import random
import py

from os import *
from stat import *
from errno import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

# This test is similar to symlink_01 except it remounts the
# filesystem in the middle so we can check persistence.

class Test_fs_symlink_02:

  def setup_class(self):
    self.bspath = "bs_symlink_02.bs"

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_symlink(self):

    # Remove any prexisting blockstore.
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

    # Create the filesystem
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    bsargs)
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

    # Now we unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()
	
    # Now mount it again.
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, "rootbs", bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

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
    self.bs.bs_close()
    self.bs = None
    self.fs = None
