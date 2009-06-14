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

class Test_fs_persist_01:

  def setup_class(self):
    self.bspath = "fs_persist_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "", "", CONFIG.FSARGS)

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_persistence(self):
    # Create a file.
    self.fs.fs_mknod("/foo", 0666, 0)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
    
    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # We should be able to stat the same file.
    st = self.fs.fs_getattr("/foo");
    assert S_ISREG(st[ST_MODE])
