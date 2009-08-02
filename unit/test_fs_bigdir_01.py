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

import utp.PyDirEntryFunc

# This test checks that directories can have lots of items.

# This callback counts entries.
class DirEntryCounter(utp.PyDirEntryFunc.PyDirEntryFunc):
  def __init__(self):
    utp.PyDirEntryFunc.PyDirEntryFunc.__init__(self)
    self.count = 0

  def def_entry(self, name, statbuf, offset):
    self.count += 1

  def count(self):
    return self.count

class Test_fs_bigdir_01:

  def setup_class(self):
    self.bspath = "fs_bigdir_01.bs"

  def teardown_class(self):
    CONFIG.remove_bs(self.bspath)

  def test_bigdir(self):

    # Remove any prexisting blockstore.
    CONFIG.remove_bs(self.bspath)

    # Create the filesystem
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, CONFIG.BSSIZE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a directory.
    self.fs.fs_mkdir("/bigdir", 0555, CONFIG.UNAME, CONFIG.GNAME)

    # Create lots of files in the directory.
    nfiles = 1024
    for ndx in range(0, nfiles):
      path = "/bigdir/%04d" % (ndx,)
      self.fs.fs_mknod(path, 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Count the directory entries.
    cntr = DirEntryCounter()
    self.fs.fs_readdir("/bigdir", 0, cntr)
    assert cntr.count == nfiles + 2

    # Sync the filesystem.
    self.fs.fs_sync()

    # Count the directory entries again.
    cntr = DirEntryCounter()
    self.fs.fs_readdir("/bigdir", 0, cntr)
    assert cntr.count == nfiles + 2

    # Now we unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()

    # Now mount it again.
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Count the directory entries.
    cntr = DirEntryCounter()
    self.fs.fs_readdir("/bigdir", 0, cntr)
    assert cntr.count == nfiles + 2

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None
