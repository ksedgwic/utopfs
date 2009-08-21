import sys
import random
import py
import time

import utp
import utp.BlockStore

import CONFIG
from lenhack import *

# This test makes sure that get operation results get copied across
# all child blockstores.

class Test_vbs_head_02:

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


  def test_on_delta(self):
    # First child.
    bspath1 = "vbs_head_02_c1"
    CONFIG.unmap_bs("child1")
    CONFIG.remove_bs(bspath1)
    self.bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child1",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath1))

    # Second child.
    bspath2 = "vbs_head_02_c2"
    CONFIG.unmap_bs("child2")
    CONFIG.remove_bs(bspath2)
    self.bs2 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child2",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath2))
    
    # Third child.
    bspath3 = "vbs_head_02_c3"
    CONFIG.unmap_bs("child3")
    CONFIG.remove_bs(bspath3)
    self.bs3 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child3",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath3))

    # Open the virtual block store.
    CONFIG.unmap_bs("rootbs")
    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1", "child2", "child3"))

    # Insert a single SHN.
    node1 = utp.SignedHeadNode(("fsid", "node1", 0, time.time() * 1e6, 0, 0))
    self.vbs.bs_head_insert(node1)

    # Insert a child SHN.
    node2 = utp.SignedHeadNode(("fsid", "node2", "node1", time.time() * 1e6, 0, 0))
    self.vbs.bs_head_insert(node2)

    # Insert a new node into one child only.
    node2 = utp.SignedHeadNode(("fsid", "node3", "node2", time.time() * 1e6, 0, 0))
    self.bs3.bs_head_insert(node2)

    # Follow w/ empty should return all nodes.
    shns = self.vbs.bs_head_follow(node0)
    assert lenhack(shns) == 3
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref),
                   str(shns[2].rootref))) == ["node1", "node2", "node3"]

    # Further w/ empty should return all nodes.
    shns = self.vbs.bs_head_further(node0)
    assert lenhack(shns) == 3
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref),
                   str(shns[2].rootref))) == ["node1", "node2", "node3"]

    # Close and reopen without the special child.
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
    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1", "child2"))

    # Follow w/ empty should return all nodes.
    shns = self.vbs.bs_head_follow(node0)
    assert lenhack(shns) == 3
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref),
                   str(shns[2].rootref))) == ["node1", "node2", "node3"]

    # Further w/ empty should return all nodes.
    shns = self.vbs.bs_head_further(node0)
    assert lenhack(shns) == 3
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref),
                   str(shns[2].rootref))) == ["node1", "node2", "node3"]

    # Close for good.
    self.vbs.bs_close()
    self.vbs = None
    self.bs2.bs_close()
    self.bs2 = None
    self.bs1.bs_close()
    self.bs1 = None
    CONFIG.remove_bs(bspath2)
    CONFIG.remove_bs(bspath1)
