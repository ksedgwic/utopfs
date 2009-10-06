import sys
import random
import py
import time

import utp
import utp.BlockStore
import utp.FileSystem

import CONFIG
from lenhack import *

# This test make sure we can bootstrap a filesystem into an
# uninitialized blockstore from an initialized blockstore.

class Test_vbs_head_03:

  def setup_class(self):
    self.bs1 = None
    self.bs2 = None
    self.vbs = None
    pass

  def teardown_class(self):
    if self.vbs:
      self.vbs.bs_close()
      self.vbs = None
    if self.bs2:
      self.bs2.bs_close()
      self.bs2 = None
    if self.bs1:
      self.bs1.bs_close()
      self.bs1 = None


  def test_on_delta(self):
    print "Create first child."
    bspath1 = "vbs_head_03_c1"
    CONFIG.unmap_bs("child1")
    CONFIG.remove_bs(bspath1)
    self.bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child1",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath1))

    print "Make a filesystem on this blockstore."
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs1, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    print "Unmount the filesystem."
    self.fs.fs_umount()

    print "Remount the filesystem."
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs1, "", "",
                                   CONFIG.FSARGS)

    print "Create a file."
    self.fs.fs_mknod("/bar", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    print "Sync the filesystem."
    self.fs.fs_sync()

    print "Unmount."
    self.fs.fs_umount()

    print "Create second child."
    bspath2 = "vbs_head_03_c2"
    CONFIG.unmap_bs("child2")
    CONFIG.remove_bs(bspath2)
    self.bs2 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child2",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath2))
    
    print "Open the virtual block store."
    CONFIG.unmap_bs("rootbs")
    self.vbs = utp.BlockStore.open("VBS", "rootbs", ("child1", "child2"))

    print "Mount the filesystem on the VBS."
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.vbs, "", "",
                                   CONFIG.FSARGS)

    print "Now we should be able to stat the file."
    self.fs.fs_getattr("/bar")

    print "Run a refresh cycle"
    self.fs.fs_refresh()
    
    print "Make sure everything is done transferring."
    self.vbs.bs_sync()

    print "Unmount."
    self.fs.fs_umount()

    print "Mount the filesystem on the second child alone."
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs2, "", "",
                                   CONFIG.FSARGS)

    print "Now we should be able to stat the file."
    self.fs.fs_getattr("/bar");
    
    print "Unmount."
    self.fs.fs_umount()

    print "Close for good."
    self.vbs.bs_close()
    self.vbs = None
    self.bs2.bs_close()
    self.bs2 = None
    self.bs1.bs_close()
    self.bs1 = None
    CONFIG.remove_bs(bspath2)
    CONFIG.remove_bs(bspath1)
