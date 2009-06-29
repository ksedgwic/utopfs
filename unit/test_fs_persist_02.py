import sys
import random
import py


from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

class Test_fs_persist_02:

  def setup_class(self):
    self.bspath = "fs_persist_02.bs"

  def teardown_class(self):
    CONFIG.remove_bs(self.bspath) 

  def test_persistence(self):
    # Remove any prexisting blockstore.
    CONFIG.remove_bs(self.bspath)  

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a diretory.
    self.fs.fs_mkdir("/foo", 0755, CONFIG.UNAME, CONFIG.GNAME)

    # Create a file in the directory.
    self.fs.fs_mknod("/foo/bar", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Now we should be able to stat the directory.
    st = self.fs.fs_getattr("/foo");
    assert S_ISDIR(st[ST_MODE])
    
    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/foo/bar");
    assert S_ISREG(st[ST_MODE])
    
    # Now we unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Now we should be able to stat the directory.
    st = self.fs.fs_getattr("/foo");
    assert S_ISDIR(st[ST_MODE])
    
    # We should be able to stat the same file.
    st = self.fs.fs_getattr("/foo/bar");
    assert S_ISREG(st[ST_MODE])

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None
