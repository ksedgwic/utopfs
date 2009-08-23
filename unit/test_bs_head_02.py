import sys
import random
import py
import utp
import utp.BlockStore

import time

import CONFIG
from lenhack import *

# This test is for basic multiple node operations.

class Test_bs_head_02:

  def setup_class(self):
    self.bspath = "bs_head_02"
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def teardown_class(self):
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)

  def test_two_nodes(self):

    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))

    # Insert the first SHN.
    node1 = utp.SignedHeadEdge(("fsid", "node1", 0,
                               time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node1)

    # Insert the second SHN.
    node2 = utp.SignedHeadEdge(("fsid", "node2", "node1",
                               time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node2)

    # Furthest w/ empty should return the second node we inserted.
    seed0 = (buffer("fsid"), buffer(""))
    shes = self.bs.bs_head_furthest(seed0)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node2"))

    # Furthest w/ first should return the second node we inserted.
    seed1 = (buffer("fsid"), buffer("node1"))
    shes = self.bs.bs_head_furthest(seed1)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node2"))

    # Furthest w/ last should return the second node we inserted.
    seed2 = (buffer("fsid"), buffer("node2"))
    shes = self.bs.bs_head_furthest(seed2)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node2"))

    # Follow w/ empty should return both nodes.
    shes = self.bs.bs_head_follow(seed0)
    assert lenhack(shes) == 2
    assert str(shes[0].rootref) == "node1"
    assert str(shes[1].rootref) == "node2"

    # Follow w/ first should return the second node.
    shes = self.bs.bs_head_follow(seed1)
    assert lenhack(shes) == 1
    assert str(shes[0].rootref) == "node2"

    # Follow w/ last should return nothing.
    shes = self.bs.bs_head_follow(seed2)
    assert lenhack(shes) == 0

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Furthest w/ empty should return the second node we inserted.
    seed0 = (buffer("fsid"), buffer(""))
    shes = self.bs.bs_head_furthest(seed0)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node2"))

    # Furthest w/ first should return the second node we inserted.
    seed1 = (buffer("fsid"), buffer("node1"))
    shes = self.bs.bs_head_furthest(seed1)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node2"))

    # Furthest w/ last should return the second node we inserted.
    seed2 = (buffer("fsid"), buffer("node2"))
    shes = self.bs.bs_head_furthest(seed2)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node2"))

    # Follow w/ empty should return both nodes.
    shes = self.bs.bs_head_follow(seed0)
    assert lenhack(shes) == 2
    assert str(shes[0].rootref) == "node1"
    assert str(shes[1].rootref) == "node2"

    # Follow w/ first should return the second node.
    shes = self.bs.bs_head_follow(seed1)
    assert lenhack(shes) == 1
    assert str(shes[0].rootref) == "node2"

    # Follow w/ last should return nothing.
    shes = self.bs.bs_head_follow(seed2)
    assert lenhack(shes) == 0

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)
