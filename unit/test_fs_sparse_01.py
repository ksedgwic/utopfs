import sys
import random
import py
from os import *
from stat import *

import shutil
import utp
import utp.FileSystem
import utp.PyDirEntryFunc

# This test ensures that sparse file support works.

class Test_fs_sparse_01:

  def setup_class(self):
    self.bspath = "fs_sparse_01.bs"

    # Remove any prexisting blockstore.
    shutil.rmtree(self.bspath,True)  

    # Find the FileSystem singleton.
    self.fs = utp.FileSystem.instance()
    
    # Make the filesystem.
    self.fs.fs_mkfs(self.bspath, "")

  def teardown_class(self):
    shutil.rmtree(self.bspath,True) 

  def test_can_write_sparse(self):

    # Create a new file.
    self.fs.fs_mknod("/sparse", 0666, 0)

    # Create a 18000 byte string"
    jnkstr = ""
    for i in range(0, 2000):
      jnkstr += "%08d\n" % (i)
    assert jnkstr[0:8] == "00000000"
    assert jnkstr[18000-9:18000-1] == "00001999"

    # Write the buffer to the file.
    rv = self.fs.fs_write("/sparse", buffer(jnkstr))
    assert rv == 18000

    # Stat should show the size.
    st = self.fs.fs_getattr("/sparse")
    assert st.st_size == 18000

    # Check the number of file blocks.
    # inode + 2 data blocks * 8192 / 512
    nblocks = st.st_blocks
    assert nblocks == (1 + 2) * 8192 / 512

    # Write the buffer at 1Mbyte offset.
    rv = self.fs.fs_write("/sparse", buffer(jnkstr), 1024 * 1024)

    # Size should include the whole space.
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + (1024 * 1024)

    # But the number of blocks is much less ...
    # inode + 2 data blocks + indirect + 3 data blocks
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 + 1 + 3) * 8192 / 512

    # We should be able to read the data.
    buf = self.fs.fs_read("/sparse", 18000, 1024 * 1024)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    # Reading somewhere in the "hole" should see zeros.
    buf = self.fs.fs_read("/sparse", 18000, 512 * 1024)
    assert str(buf)[0] == '\0'
    assert str(buf)[18000-1] == '\0'

    # And the read shouldn't change the size or block count.
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + (1024 * 1024)
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 + 1 + 3) * 8192 / 512

    # Now we unmount the filesystem.
    self.fs.fs_unmount()

    # Now mount it again.
    self.fs.fs_mount(self.bspath, "")

    # Size should include the whole space.
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + (1024 * 1024)

    # But the number of blocks is much less ...
    # inode + 2 data blocks + indirect + 3 data blocks
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 + 1 + 3) * 8192 / 512

    # We should be able to read the data.
    self.fs.fs_open("/sparse", O_RDONLY)
    buf = self.fs.fs_read("/sparse", 18000, 1024 * 1024)
    assert str(buf)[0:8] == "00000000"
    assert str(buf)[18000-9:18000-1] == "00001999"

    # Reading somewhere in the "hole" should see zeros.
    buf = self.fs.fs_read("/sparse", 18000, 512 * 1024)
    assert str(buf)[0] == '\0'
    assert str(buf)[18000-1] == '\0'

    # And the read shouldn't change the size or block count.
    st = self.fs.fs_getattr("/sparse")
    nbytes = st.st_size
    assert nbytes == 18000 + (1024 * 1024)
    nblocks = st.st_blocks
    assert nblocks == (1 + 2 + 1 + 3) * 8192 / 512