import sys
import random
import py
import utp
import utp.BlockStore

import time

import CONFIG
from lenhack import *

# This test is for branched multiple node operations.

class Test_bs_head_03:

  def setup_class(self):
    self.bspath = "bs_head_03"
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
    node1 = utp.SignedHeadNode(("fsid", "node1", 0,
                               time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node1)

    # Insert the second SHN.
    node2 = utp.SignedHeadNode(("fsid", "node2", "node1",
                               time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node2)

    # The third SHN is a branch.
    node3 = utp.SignedHeadNode(("fsid", "node3", "node1",
                               time.time() * 1e6, 0, 0))
    self.bs.bs_head_insert(node3)

    # Furthest w/ empty should return the two branch heads.
    node0 = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    shns = self.bs.bs_head_furthest(node0)
    assert lenhack(shns) == 2
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref))) == ["node2", "node3"]

    # Furthest w/ first should return the two branch heads.
    shns = self.bs.bs_head_furthest(node1)
    assert lenhack(shns) == 2
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref))) == ["node2", "node3"]

    # Furthest w/ one branch should return that branch.
    shns = self.bs.bs_head_furthest(node2)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "node2"

    # Follow w/ empty should return all nodes.
    shns = self.bs.bs_head_follow(node0)
    assert lenhack(shns) == 3
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref),
                   str(shns[2].rootref))) == ["node1", "node2", "node3"]

    # Follow w/ first should return all nodes except the starting.
    shns = self.bs.bs_head_follow(node1)
    assert lenhack(shns) == 2
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref))) == ["node2", "node3"]

    # Follow w/ branch should return that branch only.
    shns = self.bs.bs_head_furthest(node3)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "node3"

    # Reopen the blockstore.
    self.bs.bs_close()
    self.bs = utp.BlockStore.open(CONFIG.BSTYPE,
                                  "rootbs",
                                  CONFIG.BSARGS(self.bspath))

    # Furthest w/ empty should return the two branch heads.
    node0 = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    shns = self.bs.bs_head_furthest(node0)
    assert lenhack(shns) == 2
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref))) == ["node2", "node3"]

    # Furthest w/ first should return the two branch heads.
    shns = self.bs.bs_head_furthest(node1)
    assert lenhack(shns) == 2
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref))) == ["node2", "node3"]

    # Furthest w/ one branch should return that branch.
    shns = self.bs.bs_head_furthest(node2)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "node2"

    # Follow w/ empty should return all nodes.
    shns = self.bs.bs_head_follow(node0)
    assert lenhack(shns) == 3
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref),
                   str(shns[2].rootref))) == ["node1", "node2", "node3"]

    # Follow w/ first should return all nodes except the starting.
    shns = self.bs.bs_head_follow(node1)
    assert lenhack(shns) == 2
    assert sorted((str(shns[0].rootref),
                   str(shns[1].rootref))) == ["node2", "node3"]

    # Follow w/ branch should return that branch only.
    shns = self.bs.bs_head_furthest(node3)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "node3"

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)
