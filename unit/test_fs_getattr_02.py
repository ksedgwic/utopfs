import sys
import random
import py


from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

# Similar to fs_getattr_01 but tests after re-mounting.

class Test_fs_getattr_01:

  def setup_class(self):
    self.bspath = "fs_getattr_01.bs"

    # Remove any prexisting blockstore.
    CONFIG.remove_bs(self.bspath)

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Make a directory.
    self.fs.fs_mkdir("/foo", 0755, CONFIG.UNAME, CONFIG.GNAME)

    # Make a file.
    self.fs.fs_mknod("/foo/bar", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Now we unmount the filesystem.
    self.fs.fs_umount()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)


  def teardown_class(self):
    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    olvl = utp.FileSystem.loglevel(-1)
    self.bs = None
    self.fs = None
    utp.FileSystem.loglevel(olvl)

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
