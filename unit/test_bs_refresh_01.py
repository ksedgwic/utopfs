import sys
import random
import py

import utp
import utp.BlockStore

import CONFIG
from lenhack import *

class Test_bs_refresh_01:

  def setup_class(self):
    self.bspath = "bs_refresh_01"
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))
    
  def teardown_class(self):
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)

  def test_refresh_nostart(self):
    # Need a start
    keys = (buffer('key1'), buffer('key3'))
    py.test.raises(utp.NotFoundError,
                   self.bs.bs_refresh_blocks, 777, keys)

    # Same
    py.test.raises(utp.NotFoundError,
                   self.bs.bs_refresh_finish, 777)

  def test_refresh_dupstart(self):
    self.bs.bs_refresh_start(778)
    py.test.raises(utp.NotUniqueError,
                   self.bs.bs_refresh_start, 778)

  def test_refresh_blocks(self):

    # store 3 blocks
    for keystr in ('key1', 'key2', 'key3'):
      k = buffer(keystr)
      v = buffer("this phrase is data")
      self.bs.bs_block_put(k, v)

    # refresh 2 of them
    keys = (buffer('key1'), buffer('key3'))
    self.bs.bs_refresh_start(42)
    missing = self.bs.bs_refresh_blocks(42, keys)
    self.bs.bs_refresh_finish(42)

    assert lenhack(missing) == 0

  def test_refresh_missing(self):

    # store 3 blocks
    for keystr in ('key1', 'key2', 'key3'):
      k = buffer(keystr)
      v = buffer("this phrase is also data")
      self.bs.bs_block_put(k, v)

    # refresh 1 of them and one that isn't there
    keys = (buffer('key2'), buffer('key4'))
    self.bs.bs_refresh_start(714)
    missing = self.bs.bs_refresh_blocks(714, keys)
    self.bs.bs_refresh_finish(714)
    assert lenhack(missing) == 1
    assert missing[0] == buffer('key4')
