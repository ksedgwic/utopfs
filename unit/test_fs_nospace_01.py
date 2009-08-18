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

class Test_fs_nospace_01:

  def setup_class(self):
    self.bspath = "fs_nospace_01.bs"

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_nospace(self):

    # Remove any prexisting blockstore.
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

    # Create a small filesystem
    bssz = 64 * 1024
    bsargs = CONFIG.BSARGS(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    bssz,
                                    bsargs)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == bssz
    assert bss.bss_free == bssz

    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a file.
    self.fs.fs_mknod("/toolarge", 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Write something that is too large. Won't fail immediately due to
    # cache ...
    buf = ""
    for i in range(0, 100 * 1024):
      buf += "x"
    self.fs.fs_write("/toolarge", buffer(buf))

    # Attempt to sync the filesystem.
    py.test.raises(utp.NoSpaceError, self.fs.fs_sync)

    # Unmounting has the same problem.
    py.test.raises(utp.NoSpaceError, self.fs.fs_umount)

    # Delete the file.
    self.fs.fs_unlink("/toolarge")

    # Now we need to compact.
    self.fs.fs_refresh();

    # Now we can sync.
    self.fs.fs_sync();

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

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs.bs_close()
    self.bs = None
    self.fs = None
