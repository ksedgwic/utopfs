import sys
import random
import py
import utp
import utp.BlockStore

import time

import CONFIG
from lenhack import *

# This test is for merged multiple node operations.

class Test_bs_head_04:

  def setup_class(self):
    self.bspath = "bs_head_04"
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
    node2a = utp.SignedHeadEdge(("fsid", "node2a", "node1",
                               time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node2a)

    # Another node is a branch.
    node2b = utp.SignedHeadEdge(("fsid", "node2b", "node1",
                               time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node2b)

    # A merge node.
    node3a = utp.SignedHeadEdge(("fsid", "node3", "node2a",
                                 time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node3a)

    # Another merge node.
    node3b = utp.SignedHeadEdge(("fsid", "node3", "node2b",
                                 time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node3b)

    # Furthest w/ empty should return the the merge head.
    seed0 = (buffer("fsid"), buffer(""))
    shes = self.bs.bs_head_furthest(seed0)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node3"))

    # Furthest w/ first should return the merge head.
    seed1 = (buffer("fsid"), buffer("node1"))
    shes = self.bs.bs_head_furthest(seed1)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node3"))

    # Furthest w/ one branch should return the merge head.
    seed2a = (buffer("fsid"), buffer("node2a"))
    shes = self.bs.bs_head_furthest(seed2a)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node3"))

    # Follow w/ empty should return all nodes.
    shes = self.bs.bs_head_follow(seed0)
    assert lenhack(shes) == 5
    assert sorted((str(shes[0].rootref),
                   str(shes[1].rootref),
                   str(shes[2].rootref),
                   str(shes[3].rootref),
                   str(shes[4].rootref))) == ["node1",
                                              "node2a", "node2b",
                                              "node3",  "node3"]

    # Follow w/ first should return all nodes except the starting.
    shes = self.bs.bs_head_follow(seed1)
    assert lenhack(shes) == 4
    assert sorted((str(shes[0].rootref),
                   str(shes[1].rootref),
                   str(shes[2].rootref),
                   str(shes[3].rootref))) == ["node2a", "node2b",
                                              "node3",  "node3"]

    # Follow w/ branch should return that branch only.
    seed2b = (buffer("fsid"), buffer("node2b"))
    shes = self.bs.bs_head_follow(seed2b)
    assert lenhack(shes) == 1
    assert str(shes[0].rootref) == "node3"
    assert str(shes[0].prevref) == "node2b"

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Furthest w/ empty should return the the merge head.
    shes = self.bs.bs_head_furthest(seed0)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node3"))

    # Furthest w/ first should return the merge head.
    shes = self.bs.bs_head_furthest(seed1)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node3"))

    # Furthest w/ one branch should return the merge head.
    shes = self.bs.bs_head_furthest(seed2b)
    assert lenhack(shes) == 1
    assert shes[0] == (buffer("fsid"), buffer("node3"))

    # Follow w/ empty should return all nodes.
    shes = self.bs.bs_head_follow(seed0)
    assert lenhack(shes) == 5
    assert sorted((str(shes[0].rootref),
                   str(shes[1].rootref),
                   str(shes[2].rootref),
                   str(shes[3].rootref),
                   str(shes[4].rootref))) == ["node1",
                                              "node2a", "node2b",
                                              "node3",  "node3"]

    # Follow w/ first should return all nodes except the starting.
    shes = self.bs.bs_head_follow(seed1)
    assert lenhack(shes) == 4
    assert sorted((str(shes[0].rootref),
                   str(shes[1].rootref),
                   str(shes[2].rootref),
                   str(shes[3].rootref))) == ["node2a", "node2b",
                                              "node3",  "node3"]

    # Follow w/ branch should return that branch only.
    shes = self.bs.bs_head_follow(seed2a)
    assert lenhack(shes) == 1
    assert str(shes[0].rootref) == "node3"
    assert str(shes[0].prevref) == "node2a"

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)
