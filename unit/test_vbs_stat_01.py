import sys
import random
import py

import utp
import utp.BlockStore

import CONFIG
from lenhack import *

class Test_vbs_stat_01:

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

  def test_stat_single_child(self):
    bspath1 = "vbs_stat_01_c1"
    CONFIG.remove_bs(bspath1)
    self.bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child1",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath1))

    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1",))

    # Check the blockstore stats.
    bss = self.vbs.bs_stat();
    assert bss.bss_size == CONFIG.BSSIZE
    assert bss.bss_free == CONFIG.BSSIZE

    self.vbs.bs_close()
    self.vbs = None

    self.bs1.bs_close()
    CONFIG.remove_bs(bspath1)

  def test_stat_two_children(self):
    bspath1 = "vbs_stat_01_c1"
    CONFIG.remove_bs(bspath1)
    self.bs1 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child1",
                                     CONFIG.BSSIZE,
                                     CONFIG.BSARGS(bspath1))

    # Second child is twice as big
    bspath2 = "vbs_stat_01_c2"
    CONFIG.remove_bs(bspath2)
    self.bs2 = utp.BlockStore.create(CONFIG.BSTYPE,
                                     "child2",
                                     CONFIG.BSSIZE * 2,
                                     CONFIG.BSARGS(bspath2))


    self.vbs = utp.BlockStore.open("VBS",
                                   "rootbs",
                                   ("child1", "child2"))

    # Check the blockstore stats.
    bss = self.vbs.bs_stat();
    assert bss.bss_size == 2 * CONFIG.BSSIZE
    assert bss.bss_free == 2 * CONFIG.BSSIZE

    self.vbs.bs_close()
    self.vbs = None

    self.bs2.bs_close()
    self.bs2 = None
    CONFIG.remove_bs(bspath2)

    self.bs1.bs_close()
    self.bs1 = None
    CONFIG.remove_bs(bspath1)
