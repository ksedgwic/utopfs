import sys
import random
import py

import utp
import utp.BlockStore

import CONFIG
from lenhack import *

class Test_vbs_data_01:

  def setup_class(self):
    self.bs1 = None
    self.bs2 = None
    self.vbs = None
    pass

  def teardown_class(self):
    if self.vbs:
      self.vbs.bs_close()
      self.vbs = None
    if self.bs2:
      self.bs2.bs_close()
      self.bs2 = None
    if self.bs1:
      self.bs1.bs_close()
      self.bs1 = None

  def test_data_single_child(self):
    bspath1 = "vbs_data_01_c1"
    CONFIG.unmap_bs("child1")
    CONFIG.remove_bs(bspath1)
    self.bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child1",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath1))

    CONFIG.unmap_bs("rootbs")
    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1",))

    # Put a block of data.
    key1 = buffer("key1")
    val1 = buffer("val1")
    self.vbs.bs_put_block(key1, val1)

    # Retrieve the block.
    blk1 = self.vbs.bs_get_block(key1)
    assert blk1 == val1

    # Test block that doesn't exist.
    key2 = buffer("key2")
    py.test.raises(utp.NotFoundError, self.vbs.bs_get_block, key2)

    # Close and reopen everything.
    self.vbs.bs_close()
    self.bs1.bs_close()
    self.bs1 = utp.BlockStore.open(CONFIG.BSTYPE,
                                   "child1",
                                   CONFIG.BSARGS(bspath1))
    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1", "child2"))

    # Retrieve the block.
    blk1 = self.vbs.bs_get_block(key1)
    assert blk1 == val1

    # Test block that doesn't exist.
    key2 = buffer("key2")
    py.test.raises(utp.NotFoundError, self.vbs.bs_get_block, key2)

    # Close for good.
    self.vbs.bs_close()
    self.vbs = None

    self.bs1.bs_close()
    CONFIG.remove_bs(bspath1)

  def test_data_two_children(self):
    bspath1 = "vbs_data_01_c1"
    CONFIG.unmap_bs("child1")
    CONFIG.remove_bs(bspath1)
    self.bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child1",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath1))

    # Second child is twice as big
    bspath2 = "vbs_data_01_c2"
    CONFIG.unmap_bs("child2")
    CONFIG.remove_bs(bspath2)
    self.bs2 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child2",
                                     CONFIG.BSSIZE * 2,
                                     CONFIG.BSARGS(bspath2))


    CONFIG.unmap_bs("rootbs")
    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1", "child2"))

    # Put a block of data.
    key1 = buffer("key1")
    val1 = buffer("val1")
    self.vbs.bs_put_block(key1, val1)

    # Retrieve the block.
    blk1 = self.vbs.bs_get_block(key1)
    assert blk1 == val1

    # Test block that doesn't exist.
    key2 = buffer("key2")
    py.test.raises(utp.NotFoundError, self.vbs.bs_get_block, key2)

    # Close and reopen everything.
    self.vbs.bs_close()
    self.bs2.bs_close()
    self.bs1.bs_close()
    self.bs1 = utp.BlockStore.open(CONFIG.BSTYPE,
                                   "child1",
                                   CONFIG.BSARGS(bspath1))
    self.bs2 = utp.BlockStore.open(CONFIG.BSTYPE,
                                   "child2",
                                   CONFIG.BSARGS(bspath2))
    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1", "child2"))

    # Retrieve the block.
    blk1 = self.vbs.bs_get_block(key1)
    assert blk1 == val1

    # Test block that doesn't exist.
    key2 = buffer("key2")
    py.test.raises(utp.NotFoundError, self.vbs.bs_get_block, key2)

    # Close for good.
    self.vbs.bs_close()
    self.vbs = None
    self.bs2.bs_close()
    self.bs2 = None
    self.bs1.bs_close()
    self.bs1 = None
    CONFIG.remove_bs(bspath2)
    CONFIG.remove_bs(bspath1)
