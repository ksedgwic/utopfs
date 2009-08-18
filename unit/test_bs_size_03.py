import sys
import random
import py
import utp
import utp.BlockStore

import time

import CONFIG

# These tests focus on committed vs: uncommitted blocks.

class Test_bs_size_03:

  def setup_class(self):
    self.bspath = "bs_size_03"
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    
  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_uncommitted_replaced(self):
    # In case it already exists ...
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

    # Create the blockstore with a small size.
    smallsz = 105
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    smallsz,
                                    CONFIG.BSARGS(self.bspath))

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz

    # Use a bunch of bytes
    for kval in range(1, 11):
      kstr = "k%02d" % (kval,)
      k = buffer(kstr)
      v = buffer("0123456789")
      self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # Refresh all but first block.
    time.sleep(1)
    self.bs.bs_refresh_start(714)
    time.sleep(1)
    for kval in range(2, 11):
      kstr = "k%02d" % (kval,)
      self.bs.bs_refresh_blocks(714, (buffer(kstr),))
    self.bs.bs_refresh_finish(714)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 9)

    # Uncommitted block should be there still.
    k = buffer('k01')
    v = self.bs.bs_get_block(k)
    assert v == buffer("0123456789")

    # Now we can insert another block.
    k = buffer("k11")
    v = buffer("0123456789")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # And now the uncommitted block should be missing.
    k = buffer('k01')
    py.test.raises(Exception, self.bs.bs_get_block, k)

    # Close and reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Now we can insert another block w/ the same key.
    k = buffer("k11")
    v = buffer("0123456789")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == smallsz
    assert bss.bss_free == smallsz - (10 * 10)

    # But a different key should cause NoSpaceError
    k = buffer("k13")
    v = buffer("0123456789")
    py.test.raises(utp.NoSpaceError, self.bs.bs_put_block, k, v)

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
