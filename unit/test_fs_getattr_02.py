import sys
import random
import py
from os import *
from stat import *

import shutil
import utp
import utp.FileSystem

# Similar to fs_getattr_01 but tests after re-mounting.

class Test_fs_getattr_01:

  def setup_class(self):
    self.bspath = "fs_getattr_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "", "")

    # Make a directory.
    self.fs.fs_mkdir("/foo", 0755)

    # Make a file.
    self.fs.fs_mknod("/foo/bar", 0666, 0)

    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "", "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_can_getattr_root(self):
    # We should be able to stat "/"
    st = self.fs.fs_getattr("/")
    assert S_ISDIR(st[ST_MODE])
    assert st[ST_NLINK] == 4

  def test_can_getattr_utopfs(self):
    # We should be able to stat "/.utopfs"
    st = self.fs.fs_getattr("/.utopfs")
    assert S_ISDIR(st[ST_MODE])
    assert st[ST_NLINK] == 2

  def test_can_getattr_version(self):
    # We should be able to stat "/.utopfs/version"
    st = self.fs.fs_getattr("/.utopfs/version")
    assert S_ISREG(st[ST_MODE])
    assert st[ST_NLINK] == 1

  def test_can_getattr_dir(self):
    # We should be able to stat "/foo"
    st = self.fs.fs_getattr("/foo")
    assert S_ISDIR(st[ST_MODE])
    assert st[ST_NLINK] == 2
    nblocks = st.st_blocks
    assert nblocks == 16

  def test_can_getattr_file(self):
    # We should be able to stat "/foo/bar"
    st = self.fs.fs_getattr("/foo/bar")
    assert S_ISREG(st[ST_MODE])
    assert st[ST_NLINK] == 1
    nblocks = st.st_blocks
    assert nblocks == 16
