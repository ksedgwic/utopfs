import sys
import random
import py

from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

# FIXME - This test is lame because it doesn't really change the
# owner/group of the file or directory.
#
# QUESTION - Can a non-root user usefully change the user/group?
#
# Certainly we can change the group, but this will require that
# we have several testing groups setup.
#
# For now we at least make sure the interface is there ...
#
class Test_fs_chown_01:

  def setup_class(self):
    self.bspath = "fs_chown_01.bs"

    # Remove any prexisting blockstore.
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_chown(self):

    # Create the filesystem
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Open up our umask for this experiment
    umask(0000)

    # Create a file
    self.fs.fs_mknod("/bar", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_MODE] & 0777 == 0666

    # Now we chown the file.
    self.fs.fs_chown("/bar", CONFIG.UNAME, CONFIG.GNAME)
    
    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_UID] == geteuid()
    assert st[ST_GID] == getegid()

    # Create a directory
    self.fs.fs_mkdir("/foo", 0555, CONFIG.UNAME, CONFIG.GNAME)

    # Now we chown the directory.
    self.fs.fs_chown("/foo", CONFIG.UNAME, CONFIG.GNAME)

    # Now we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");
    assert st[ST_UID] == geteuid()
    assert st[ST_GID] == getegid()

    # Now we unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()

    # Now mount it again.
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Now we should be able to stat the file.
    st = self.fs.fs_getattr("/bar");
    assert st[ST_UID] == geteuid()
    assert st[ST_GID] == getegid()

    # Now we should be able to stat the dir.
    st = self.fs.fs_getattr("/foo");
    assert st[ST_UID] == geteuid()
    assert st[ST_GID] == getegid()

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs.bs_close()
    self.bs = None
    self.fs = None
