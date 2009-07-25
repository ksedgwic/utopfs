import sys
import random
import py


from os import *
from stat import *

import CONFIG
import utp
import utp.BlockStore
import utp.FileSystem

# This test attempts to reproduce a problem with the truncation
# boundary in the indirect bocks.

def mkbuffer(ndx, bufsz):
  chr = "0123456789"[ndx % 10]
  bufstr = ""
  for i in range(0, bufsz):
    bufstr += chr
  return buffer(bufstr)

class Test_fs_truncate_03:

  def setup_class(self):
    self.bspath = "fs_truncate_03.bs"

  def teardown_class(self):
    CONFIG.remove_bs(self.bspath)

  def test_can_write_sparse(self):

    # Remove any prexisting blockstore.
    CONFIG.remove_bs(self.bspath)

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, CONFIG.BSSIZE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a new file.
    path = "/truncme"
    self.fs.fs_mknod(path, 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Fill the file w/ a bunch of data
    bufsz = 8 * 1024
    count = 256
    offset = 0
    for ndx in range(0, count):
      self.fs.fs_write(path, mkbuffer(ndx, bufsz), offset)
      offset += bufsz

    # Now truncate to down to 1600112
    newsz = 1600112
    self.fs.fs_truncate(path, newsz)

    # Make sure the size is OK
    st = self.fs.fs_getattr(path)
    nbytes = st.st_size
    assert nbytes == newsz

    # Now truncate to down to 156992
    newsz = 156992
    self.fs.fs_truncate(path, newsz)

    # Make sure the size is OK
    st = self.fs.fs_getattr(path)
    nbytes = st.st_size
    assert nbytes == newsz

    # ---------------- remount ----------------

    # Now we unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Make sure the size is OK
    st = self.fs.fs_getattr(path)
    nbytes = st.st_size
    assert nbytes == newsz

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None

