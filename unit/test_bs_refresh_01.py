import sys
import random
import py
import shutil
import utp
import utp.BlockStore

import CONFIG

class Test_bs_refresh_01:

  def setup_class(self):
    self.bspath = "bs_refresh_01"
    shutil.rmtree(self.bspath,True)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE, (self.bspath,))
    
  def teardown_class(self):
    self.bs.bs_close()
    shutil.rmtree(self.bspath,True)

  def test_refresh_blocks(self):

    # store 3 blocks
    for keystr in ('key1', 'key2', 'key3'):
      k = buffer(keystr)
      v = buffer("thevalue%(random.randrange(999999999))")
      self.bs.bs_put_block(k, v)

    # refresh 2 of them
    keys = (buffer('key1'), buffer('key3'))
    missing = self.bs.bs_refresh_blocks(keys)
    assert `len'(missing) == 0

  def test_refresh_missing(self):

    # store 3 blocks
    for keystr in ('key1', 'key2', 'key3'):
      k = buffer(keystr)
      v = buffer("thevalue%(random.randrange(999999999))")
      self.bs.bs_put_block(k, v)

    # refresh 1 of them and one that isn't there
    keys = (buffer('key2'), buffer('key4'))
    missing = self.bs.bs_refresh_blocks(keys)
    assert `len'(missing) == 1
    assert missing[0] == buffer('key4')