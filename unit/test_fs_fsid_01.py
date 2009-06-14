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

class Test_fs_fsid_01:

  def setup_class(self):
    self.bspath = "fs_fsid_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_separate_fsid(self):

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "first", "", CONFIG.FSARGS)

    # Make a file.
    self.fs.fs_mknod("/first", 0666, 0)

    # Unmount the filesystem.
    self.fs.fs_umount()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)

    # BOGUS - We're having trouble matching exceptions.
    # Replace Exception w/ NotFoundError
    #
    # Mount the filesystem w/ the wrong fsid.
    py.test.raises(Exception,
                   utp.FileSystem.mount, CONFIG.FSTYPE, self.bs,
                   "second", "", CONFIG.FSARGS)

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "first", "", CONFIG.FSARGS)

    # File should be there.
    st = self.fs.fs_getattr("/first")
