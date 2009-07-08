import sys
import random
import py
import utp
import utp.BlockStore

import time

import CONFIG

# These tests focus on running out of space.

class Test_bs_size_02:

  def setup_class(self):
    self.bspath = "bs_size_02"
    CONFIG.remove_bs(self.bspath)
    
  def teardown_class(self):
    CONFIG.remove_bs(self.bspath)

  def test_size_limited(self):
    # In case it already exists ...
    CONFIG.remove_bs(self.bspath)

    # Create the blockstore with a small size.
    smallsz = 105
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, smallsz, (self.bspath,))

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz

    # Use some bytes.
    k = buffer("k01")
    v = buffer("0123456789")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - 10

    # Use a bunch more bytes
    for kval in range(2, 11):
      kstr = "k%02d" % (kval,)
      k = buffer(kstr)
      v = buffer("0123456789")
      self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # Next put should cause an OperationError
    k = buffer("k11")
    v = buffer("0123456789")
    py.test.raises(utp.OperationError, self.bs.bs_put_block, k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # Close and reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, (self.bspath,))

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # Next put should cause an OperationError
    k = buffer("k11")
    v = buffer("0123456789")
    py.test.raises(utp.OperationError, self.bs.bs_put_block, k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)

  def test_compact(self):
    # In case it already exists ...
    CONFIG.remove_bs(self.bspath)

    # Create the blockstore with a small size.
    smallsz = 105
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, smallsz, (self.bspath,))

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz

    # Use a bunch more bytes
    for kval in range(1, 11):
      kstr = "k%02d" % (kval,)
      k = buffer(kstr)
      v = buffer("0123456789")
      self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # Next put should cause an OperationError
    k = buffer("k11")
    v = buffer("0123456789")
    py.test.raises(utp.OperationError, self.bs.bs_put_block, k, v)

    # Refresh all but first two blocks.
    time.sleep(1)
    self.bs.bs_refresh_start(714)
    time.sleep(1)
    for kval in range(3, 11):
      kstr = "k%02d" % (kval,)
      self.bs.bs_refresh_blocks(714, (buffer(kstr),))
    self.bs.bs_refresh_finish(714)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 8)

    # Now we can insert another block.
    k = buffer("k11")
    v = buffer("0123456789")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 9)

    # Close and reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE, (self.bspath,))

    # Now we can insert another block.
    k = buffer("k12")
    v = buffer("0123456789")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # Inserting the same key again should be fine.
    k = buffer("k12")
    v = buffer("0123456789")
    self.bs.bs_put_block(k, v)

    # But a different key should cause OperationError
    k = buffer("k13")
    v = buffer("0123456789")
    py.test.raises(utp.OperationError, self.bs.bs_put_block, k, v)

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)
