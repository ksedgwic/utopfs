import sys
import random
import py
import time

import utp
import utp.BlockStore
import utp.FileSystem

import CONFIG
from lenhack import *

# Make sure we transfer headnodes when one blockstore is uninitialized.

class Test_vbs_head_04:

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
    print "Create first child."
    bspath1 = "vbs_head_04_c1"
    CONFIG.unmap_bs("child1")
    CONFIG.remove_bs(bspath1)
    self.bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child1",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath1))

    # Insert a node into this child only
    node1 = utp.SignedHeadEdge(("fsid", "node1", 0, time.time() * 1e6, 0, 0))
    self.bs1.bs_head_insert(node1)

    print " Create second child."
    bspath2 = "vbs_head_04_c2"
    CONFIG.unmap_bs("child2")
    CONFIG.remove_bs(bspath2)
    self.bs2 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child2",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath2))

    # Don't insert *anything* into this child.
    
    print "Open the virtual block store."
    CONFIG.unmap_bs("rootbs")
    self.vbs = utp.BlockStore.open("VBS", "rootbs", ("child1", "child2"))

    seed0 = (buffer("fsid"), buffer(""))

    # Further w/ empty should return all nodes.
    shes = self.vbs.bs_head_furthest(seed0)
    assert lenhack(shes) == 1
    assert sorted(shes) == [ (buffer("fsid"), buffer("node1")), ]

    # Wait for synchronization to complete.
    self.vbs.bs_sync()

    # Further on second should return same result.
    shes = self.bs2.bs_head_furthest(seed0)
    assert lenhack(shes) == 1
    assert sorted(shes) == [ (buffer("fsid"), buffer("node1")), ]

    print "Close for good."
    self.vbs.bs_close()
    self.vbs = None
    self.bs2.bs_close()
    self.bs2 = None
    self.bs1.bs_close()
    self.bs1 = None
    CONFIG.remove_bs(bspath2)
    CONFIG.remove_bs(bspath1)
