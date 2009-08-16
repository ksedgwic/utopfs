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
    CONFIG.remove_bs(self.bspath)

  def teardown_class(self):
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

    # Furthest w/ empty should return the second node we inserted.
    node0 = utp.SignedHeadNode(("fsid", 0, 0, 0, 0, 0))
    shns = self.bs.bs_head_furthest(node0)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "node2"

    # Furthest w/ first should return the second node we inserted.
    shns = self.bs.bs_head_furthest(node1)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "node2"

    # Furthest w/ last should return the second node we inserted.
    shns = self.bs.bs_head_furthest(node2)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "node2"

    # Follow w/ empty should return both nodes.
    shns = self.bs.bs_head_follow(node0)
    assert lenhack(shns) == 2
    assert str(shns[0].rootref) == "node1"
    assert str(shns[1].rootref) == "node2"

    # Follow w/ first should return the second node.
    shns = self.bs.bs_head_follow(node1)
    assert lenhack(shns) == 1
    assert str(shns[0].rootref) == "node2"

    # Follow w/ last should return nothing.
    shns = self.bs.bs_head_follow(node2)
    assert lenhack(shns) == 0

    # Close the blockstore.
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)
