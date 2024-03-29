import sys
import random
import py

import utp
import utp.BlockStore

import CONFIG
from lenhack import *

# This test makes sure that VBS get operations check all child
# blockstores.

class Test_vbs_data_02:

  def setup_class(self):
    self.bs1 = None
    self.bs2 = None
    self.bs3 = None
    self.vbs = None
    pass

  def teardown_class(self):
    if self.vbs:
      self.vbs.bs_close()
      self.vbs = None
    if self.bs3:
      self.bs3.bs_close()
      self.bs3 = None
    if self.bs2:
      self.bs2.bs_close()
      self.bs2 = None
    if self.bs1:
      self.bs1.bs_close()
      self.bs1 = None

  def test_data_across_children(self):
    # First child.
    bspath1 = "vbs_data_02_c1"
    CONFIG.unmap_bs("child1")
    CONFIG.remove_bs(bspath1)
    self.bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child1",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath1))

    # Put a block of data only in the first child.
    key1 = buffer("key1")
    val1 = buffer("val1")
    self.bs1.bs_block_put(key1, val1)

    # Second child.
    bspath2 = "vbs_data_02_c2"
    CONFIG.unmap_bs("child2")
    CONFIG.remove_bs(bspath2)
    self.bs2 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child2",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath2))

    # Put a block of data only in the second child.
    key2 = buffer("key2")
    val2 = buffer("val2")
    self.bs2.bs_block_put(key2, val2)

    # Third child.
    bspath3 = "vbs_data_02_c3"
    CONFIG.unmap_bs("child3")
    CONFIG.remove_bs(bspath3)
    self.bs3 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child3",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath3))

    # Put a block of data only in the third child.
    key3 = buffer("key3")
    val3 = buffer("val3")
    self.bs3.bs_block_put(key3, val3)

    # Open the virtual block store.
    CONFIG.unmap_bs("rootbs")
    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1", "child2", "child3"))

    # Retrieve the first block.
    blk1 = self.vbs.bs_block_get(key1)
    assert blk1 == val1

    # Retrieve the second block.
    blk2 = self.vbs.bs_block_get(key2)
    assert blk2 == val2

    # Retrieve the third block.
    blk3 = self.vbs.bs_block_get(key3)
    assert blk3 == val3

    # Test block that doesn't exist.
    key4 = buffer("key4")
    py.test.raises(utp.NotFoundError, self.vbs.bs_block_get, key4)

    # Close and reopen everything.
    self.vbs.bs_close()
    self.bs3.bs_close()
    self.bs2.bs_close()
    self.bs1.bs_close()
    self.bs1 = utp.BlockStore.open(CONFIG.BSTYPE,
                                   "child1",
                                   CONFIG.BSARGS(bspath1))
    self.bs2 = utp.BlockStore.open(CONFIG.BSTYPE,
                                   "child2",
                                   CONFIG.BSARGS(bspath2))
    self.bs3 = utp.BlockStore.open(CONFIG.BSTYPE,
                                   "child3",
                                   CONFIG.BSARGS(bspath3))
    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1", "child2", "child3"))

    # Retrieve the first block.
    blk1 = self.vbs.bs_block_get(key1)
    assert blk1 == val1

    # Retrieve the second block.
    blk2 = self.vbs.bs_block_get(key2)
    assert blk2 == val2

    # Retrieve the third block.
    blk3 = self.vbs.bs_block_get(key3)
    assert blk3 == val3

    # Test block that doesn't exist.
    key4 = buffer("key4")
    py.test.raises(utp.NotFoundError, self.vbs.bs_block_get, key4)

    # Close for good.
    self.vbs.bs_close()
    self.vbs = None
    self.bs3.bs_close()
    self.bs3 = None
    self.bs2.bs_close()
    self.bs2 = None
    self.bs1.bs_close()
    self.bs1 = None
    CONFIG.remove_bs(bspath3)
    CONFIG.remove_bs(bspath2)
    CONFIG.remove_bs(bspath1)
