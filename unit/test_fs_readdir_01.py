import sys
import random
import py
import shutil

from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

import utp.PyDirEntryFunc

# This callback is constructed with a list of expected entries.  It
# returns lists of unexpected and missing entries.
#
class DirEntryChecker(utp.PyDirEntryFunc.PyDirEntryFunc):
  def __init__(self, checklist):
    utp.PyDirEntryFunc.PyDirEntryFunc.__init__(self)
    self.checkset = set(checklist)
    self.unexpected = []

  def def_entry(self, name, statbuf, offset):
    if name not in self.checkset:
      self.unexpected.append(name)
    self.checkset.remove(name)

  def missing(self):
    return self.checkset

  def unwanted(self):
    return self.unexpected

class Test_fs_readdir_01:

  def setup_class(self):
    self.bspath = "fs_readdir_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "", "", CONFIG.FSARGS)

    # Make a directory.
    self.fs.fs_mkdir("/foo", 0755)

    # Make a file.
    self.fs.fs_mknod("/foo/bar", 0666, 0)

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_can_readdir_root(self):
    # We should be able to list "/"
    dec = DirEntryChecker(['.', '..', '.utopfs', 'foo'])
    self.fs.fs_readdir("/", 0, dec)
    assert dec.missing() == set([])
    assert dec.unwanted() == []

  def test_can_readdir_utopfs(self):
    # We should be able to list "/.utopfs"
    dec = DirEntryChecker(['.', '..', 'version'])
    self.fs.fs_readdir("/.utopfs", 0, dec)
    assert dec.missing() == set([])
    assert dec.unwanted() == []

  def test_can_readdir_version(self):
    # We should not be able to list "/.utopfs/version"
    dec = DirEntryChecker([])
    py.test.raises(OSError,
                   "self.fs.fs_readdir(\"/.utopfs/version\", 0, dec)")

  def test_can_readdir_dir(self):
    # We should be able to list "/foo"
    dec = DirEntryChecker(['.', '..', 'bar'])
    self.fs.fs_readdir("/foo", 0, dec)
    assert dec.missing() == set([])
    assert dec.unwanted() == []

  def test_can_readdir_file(self):
    # We should not be able to list "/foo/bar"
    dec = DirEntryChecker([])
    py.test.raises(OSError, "self.fs.fs_readdir(\"/foo/bar\", 0, dec)")
