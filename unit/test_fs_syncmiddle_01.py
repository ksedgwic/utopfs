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

# This test checks that a sync in the middle of writes doesn't mess
# them up.

def mkbuffer(ndx, bufsz):
  chr = "0123456789"[ndx % 10]
  bufstr = ""
  for i in range(0, bufsz):
    bufstr += chr
  return buffer(bufstr)

class Test_fs_syncmiddle_01:

  def setup_class(self):
    self.bspath = "fs_syncmiddle_01.bs"

  def teardown_class(self):
    CONFIG.remove_bs(self.bspath)

  def test_syncmiddle(self):

    # Remove any prexisting blockstore.
    CONFIG.remove_bs(self.bspath)

    # Create the filesystem
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, CONFIG.BSSIZE, bsargs)
    self.fs = utp.FileSystem.mkfs(CONFIG.FSTYPE, self.bs, "", "",
                                  CONFIG.UNAME, CONFIG.GNAME, CONFIG.FSARGS)

    # Create a file.
    path = "/middle"
    self.fs.fs_mknod(path, 0666, 0, CONFIG.UNAME, CONFIG.GNAME)

    # Write some blocks.
    bufsz = 4 * 1024
    count = 2
    offset = 0
    for ndx in range(0, count):
      self.fs.fs_write(path, mkbuffer(ndx, bufsz), offset)
      offset += bufsz

    # Sync the filesystem
    self.fs.fs_sync()

    # Write some more blocks.
    for ndx in range(count, count * 2):
      self.fs.fs_write(path, mkbuffer(ndx, bufsz), offset)
      offset += bufsz

    # Now check all of the blocks.
    offset = 0
    for ndx in range(0, count * 2):
      nbuf = self.fs.fs_read(path, bufsz, offset)
      assert nbuf == mkbuffer(ndx, bufsz)
      offset += bufsz

    # Now we unmount the filesystem.
    self.fs.fs_umount()
    self.bs.bs_close()

    # Now mount it again.
    bsargs = (self.bspath,) + CONFIG.BSARGS
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, bsargs)
    self.fs = utp.FileSystem.mount(CONFIG.FSTYPE, self.bs,
                                   "", "", CONFIG.FSARGS)

    # Now check all of the blocks.
    offset = 0
    for ndx in range(0, count * 2):
      nbuf = self.fs.fs_read(path, bufsz, offset)
      assert nbuf == mkbuffer(ndx, bufsz)
      offset += bufsz

    # WORKAROUND - py.test doesn't correctly capture the DTOR logging.
    self.bs = None
    self.fs = None
