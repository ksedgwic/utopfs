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

# This tests that multiple slashes are treated as a single slash.

class Test_fs_doubleslash_01:

  def setup_class(self):
    self.bspath = "fs_doubleslash_01.bs"

  def teardown_class(self):
    CONFIG.remove_bs(self.bspath)

  def test_doubleslash(self):

    # Remove any prexisting blockstore.
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
    self.fs.fs_mkdir("/mydir", 0555, CONFIG.UNAME, CONFIG.GNAME)

    # Create a file in the directory.
    self.fs.fs_mknod("/mydir/myfile", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Make sure we can stat the file normally.
    st = self.fs.fs_getattr("/mydir/myfile")
    assert S_ISREG(st[ST_MODE])

    # Make sure we can stat the file with an extra slash.
    st = self.fs.fs_getattr("/mydir//myfile")
    assert S_ISREG(st[ST_MODE])

    # Make sure we can stat the file with a lot of extra slashes.
    st = self.fs.fs_getattr("/mydir//////myfile")
    assert S_ISREG(st[ST_MODE])
    st = self.fs.fs_getattr("/mydir///////myfile")
    assert S_ISREG(st[ST_MODE])

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None
