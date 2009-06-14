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

class Test_fs_chmod_01:

  def setup_class(self):
    self.bspath = "fs_chmod_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs,
                                  "", "", CONFIG.FSARGS)

  def teardown_class(self):
    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    utp.FileSystem.logoff()
    shutil.rmtree(self.bspath,True) 

  def test_chmod(self):

    # Open up our umask for this experiment
    umask(0000)

    # Create a file
    self.fs.fs_mknod("/bar", 0666, 0)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_MODE] & 0777 == 0666

    # Now we chmod the file.
    self.fs.fs_chmod("/bar", 0640)
    
    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_MODE] & 0777 == 0640

    # Create a directory
    self.fs.fs_mkdir("/foo", 0555)

    # Now we chmod the directory.
    self.fs.fs_chmod("/foo", 0510)
    
    # Now we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");
    assert st[ST_MODE] & 0777 == 0510

    # Now we unmount the filesystem.
    self.fs.fs_umount()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_MODE] & 0777 == 0640

    # Now we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");
    assert st[ST_MODE] & 0777 == 0510
