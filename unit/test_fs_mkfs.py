import os
import sys
import random
import py

import utp
import utp.FileSystem

bspath = "fs_mkfs.bs"

class TestBlockStore:

  def setup_class(self):
    # Remove any prexisting blockstore.
    try:
      os.unlink(bspath)
    except OSError:
      # Ignore errors, might not exist.
      pass

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(bspath)

  def teardown_class(self):
    os.unlink(bspath)

  def test_hasno_foodir(self):
    # The filesystem should not have a "foo" directory.
    py.test.raises(OSError, "self.fs.fs_getattr('/foo')")

  def test_has_dotdir(self):
    # The filesystem should have a .utopfs directory.
    statbuf = self.fs.fs_getattr("/.utopfs")

    # The filesystem should have a version file.
    statbuf = self.fs.fs_getattr("/.utopfs/version")

    # The version file should contain a string.
    self.fs.fs_open("/.utopfs/version", os.O_RDONLY)
    data = self.fs_fs_read("/.utopfs/version", 100, 0)


