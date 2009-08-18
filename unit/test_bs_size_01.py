import sys
import random
import py
import utp
import utp.BlockStore

import time

import CONFIG

class Test_bs_size_01:

  def setup_class(self):
    self.bspath = "bs_size_01"
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    
  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_bs_stat(self):
    # In case it already exists ...
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

    # Create the blockstore.
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE

    # Check that it looks like a tuple too.
    assert bss == (CONFIG.BSSIZE, CONFIG.BSSIZE)

    # Use some bytes.
    k = buffer("samplekey")
    v = buffer("sampledata")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE - 10

    # Use some more bytes.
    k = buffer("samplekey2")
    v = buffer("sampledata2")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE - 21

    # Close and reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE - 21

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)

  def test_refresh_changes_free(self):
    # In case it already exists ...
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

    # Create the blockstore.
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE

    # Use some bytes.
    k = buffer("samplekey")
    v = buffer("sampledata")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE - 10

    # Use some more bytes.
    k = buffer("samplekey2")
    v = buffer("sampledata2")
    self.bs.bs_put_block(k, v)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE - 21

    # Perform a refresh cycle, only refresh the 2nd block.
    #
    # This is LAME, we need to sleep so the tstamps will
    # be on a different second.
    #
    time.sleep(1)
    self.bs.bs_refresh_start(714)
    time.sleep(1)
    self.bs.bs_refresh_blocks(714, (buffer("samplekey2"),))
    self.bs.bs_refresh_finish(714)

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE - 11

    # Close and reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Check the blockstore stats.
    bss = self.bs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE - 11

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)
