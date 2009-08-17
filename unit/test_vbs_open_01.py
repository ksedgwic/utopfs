import sys
import random
import py

import utp
import utp.BlockStore

import CONFIG
from lenhack import *

class Test_bs_refresh_01:

  def setup_class(self):
    pass

  def teardown_class(self):
    pass

  def test_open_single_child(self):
    bspath1 = "vbs_open_01_c1"
    CONFIG.remove_bs(bspath1)
    bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                "child1",
                                CONFIG.BSSIZE,
                                CONFIG.BSARGS(bspath1))

    vbs = utp.BlockStore.open("VBS",
                              "rootbs",
                              ("child1",))
    vbs.bs_close()

    bs1.bs_close()
    CONFIG.remove_bs(bspath1)

  def test_open_two_children(self):
    bspath1 = "vbs_open_01_c1"
    CONFIG.remove_bs(bspath1)
    bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                "child1",
                                CONFIG.BSSIZE,
                                CONFIG.BSARGS(bspath1))

    bspath2 = "vbs_open_01_c2"
    CONFIG.remove_bs(bspath2)
    bs2 = utp.BlockStore.create(CONFIG.BSTYPE,
                                "child2",
                                CONFIG.BSSIZE,
                                CONFIG.BSARGS(bspath2))


    vbs = utp.BlockStore.open("VBS",
                              "rootbs",
                              ("child1", "child2"))

    vbs.bs_close()

    bs2.bs_close()
    CONFIG.remove_bs(bspath2)

    bs1.bs_close()
    CONFIG.remove_bs(bspath1)

  def test_open_three_children(self):
    bspath1 = "vbs_open_01_c1"
    CONFIG.remove_bs(bspath1)
    bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                "child1",
                                CONFIG.BSSIZE,
                                CONFIG.BSARGS(bspath1))

    bspath2 = "vbs_open_01_c2"
    CONFIG.remove_bs(bspath2)
    bs2 = utp.BlockStore.create(CONFIG.BSTYPE,
                                "child2",
                                CONFIG.BSSIZE,
                                CONFIG.BSARGS(bspath2))

    bspath3 = "vbs_open_01_c3"
    CONFIG.remove_bs(bspath3)
    bs3 = utp.BlockStore.create(CONFIG.BSTYPE,
                                "child3",
                                CONFIG.BSSIZE,
                                CONFIG.BSARGS(bspath3))


    vbs = utp.BlockStore.open("VBS",
                              "rootbs",
                              ("child1", "child2", "child3"))

    vbs.bs_close()


    bs3.bs_close()
    CONFIG.remove_bs(bspath3)

    bs2.bs_close()
    CONFIG.remove_bs(bspath2)

    bs1.bs_close()
    CONFIG.remove_bs(bspath1)

  def test_open_with_missing(self):
    bspath1 = "vbs_open_01_c1"
    CONFIG.remove_bs(bspath1)
    bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                "child1",
                                CONFIG.BSSIZE,
                                CONFIG.BSARGS(bspath1))

    # The second child is missing.

    bspath3 = "vbs_open_01_c3"
    CONFIG.remove_bs(bspath3)
    bs3 = utp.BlockStore.create(CONFIG.BSTYPE,
                                "child3",
                                CONFIG.BSSIZE,
                                CONFIG.BSARGS(bspath3))

    py.test.raises(utp.NotFoundError,
                   utp.BlockStore.open,
                   "VBS",
                   "rootbs",
                   ("child1", "child2", "child3"))

    bs3.bs_close()
    CONFIG.remove_bs(bspath3)

    bs1.bs_close()
    CONFIG.remove_bs(bspath1)
