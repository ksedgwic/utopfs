import sys
import random
import py
from os import *
from stat import *

import shutil
import utp
import utp.FileSystem
import utp.PyDirEntryFunc

# This test ensures that multiple block files are working correctly.

class Test_fs_bigfile_01:

  def setup_class(self):
    self.bspath = "fs_bigfile_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_can_write_bigfile(self):

    # Create a new file.
    self.fs.fs_mknod("/bigfile", 0666, 0)

    # Create a 18000 byte string"
    jnkstr = ""
    for i in range(0, 2000):
      jnkstr += "%08d\n" % (i)
    assert jnkstr[0:8] == "00000000"
    assert jnkstr[18000-9:18000-1] == "00001999"

    # Write the buffer to the file.
    rv = self.fs.fs_write("/bigfile", buffer(jnkstr))
    assert rv == 18000

    # Stat should show the size.
    st = self.fs.fs_getattr("/bigfile");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 18000
    nblocks = st.st_blocks
    assert nblocks == (1 + 2) * 8192 / 512

    # We should be able to read the data.
    self.fs.fs_open("/bigfile", O_RDONLY)
    buf = self.fs.fs_read("/bigfile", 18000)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "")

    # Stat should show the size.
    st = self.fs.fs_getattr("/bigfile");
    assert S_ISREG(st[ST_MODE])
    assert st[ST_SIZE] == 18000
    nblocks = st.st_blocks
    assert nblocks == (1 + 2) * 8192 / 512

    # We should be able to read the data.
    self.fs.fs_open("/bigfile", O_RDONLY)
    buf = self.fs.fs_read("/bigfile", 18000)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"
