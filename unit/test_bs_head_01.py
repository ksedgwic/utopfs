import sys
import random
import py
import utp
import utp.BlockStore

import time

import CONFIG
from lenhack import *

# This test is for basic API and simple edge conditions.

class Test_bs_head_01:

  def setup_class(self):
    self.bspath = "bs_head_01"
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_furthest_on_empty(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Furthest should generate NotFound on empty BS.
    seed = utp.SignedHeadNode((0, 0, 0, 0, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_furthest, seed)

    # Furthest should generate NotFound on empty BS, even w/ fsid
    seed = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_furthest, seed)

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Furthest should generate NotFound on empty BS.
    seed = utp.SignedHeadNode((0, 0, 0, 0, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_furthest, seed)

    # Furthest should generate NotFound on empty BS, even w/ fsid
    seed = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_furthest, seed)

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_furthest_on_single(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Insert a single SHN.
    node = utp.SignedHeadNode(("fsid", "rootref", 0, time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node)

    # Furthest should return the node we inserted.
    seed = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    shns = self.bs.bs_head_furthest(seed)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "rootref"

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Furthest should return the node we inserted.
    seed = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    shns = self.bs.bs_head_furthest(seed)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "rootref"

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_furthest_on_single_w_seed(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Insert a single SHN.
    node = utp.SignedHeadNode(("fsid", "rootref", 0, time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node)

    # Furthest should return the node we inserted.
    shns = self.bs.bs_head_furthest(node)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "rootref"

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Furthest should return the node we inserted.
    shns = self.bs.bs_head_furthest(node)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "rootref"

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_furthest_on_single_w_miss(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Insert a single SHN.
    node = utp.SignedHeadNode(("fsid", "rootref", 0, time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node)

    # Furthest should miss w/ bad key.
    miss = utp.SignedHeadNode(("fsid", "notref", 0, time.time() * 1e6, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_furthest, miss)

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Furthest should miss w/ bad key.
    miss = utp.SignedHeadNode(("fsid", "notref", 0, time.time() * 1e6, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_furthest, miss)

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_follow_on_empty(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Follow should generate NotFound on empty BS.
    seed = utp.SignedHeadNode((0, 0, 0, 0, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_follow, seed)

    # Follow should generate NotFound on empty BS, even w/ fsid
    seed = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_follow, seed)

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Follow should generate NotFound on empty BS.
    seed = utp.SignedHeadNode((0, 0, 0, 0, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_follow, seed)

    # Follow should generate NotFound on empty BS, even w/ fsid
    seed = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_follow, seed)

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_follow_on_single(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Insert a single SHN.
    node = utp.SignedHeadNode(("fsid", "rootref", 0, time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node)

    # Follow should return the node we inserted.
    seed = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    shns = self.bs.bs_head_follow(seed)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "rootref"

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Follow should return the node we inserted.
    seed = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    shns = self.bs.bs_head_follow(seed)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "rootref"

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_follow_on_single_w_seed(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Insert a single SHN.
    node = utp.SignedHeadNode(("fsid", "rootref", 0, time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node)

    # Follow should return the empty set
    shns = self.bs.bs_head_follow(node)
    assert lenhack(shns) == 0

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Follow should return the empty set
    shns = self.bs.bs_head_follow(node)
    assert lenhack(shns) == 0

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_follow_on_single_w_miss(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Insert a single SHN.
    node = utp.SignedHeadNode(("fsid", "rootref", 0, time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node)

    # Follow should miss w/ bad key.
    miss = utp.SignedHeadNode(("fsid", "notref", 0, time.time() * 1e6, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_follow, miss)

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Follow should miss w/ bad key.
    miss = utp.SignedHeadNode(("fsid", "notref", 0, time.time() * 1e6, 0, 0))
    py.test.raises(utp.NotFoundError, self.bs.bs_head_follow, miss)

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

