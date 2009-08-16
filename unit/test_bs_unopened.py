import sys
import random
import py
import os
import utp
import utp.BlockStore

import CONFIG

class TestUnopenedBlockStore:

  def setup_class(self):
    self.bspath = "bs_unopened.bs"
    CONFIG.remove_bs(self.bspath)

  def teardown_class(self):
    pass
    
  def test_open_on_nonexistent(self):
    py.test.raises(utp.NotFoundError,
                   utp.BlockStore.open,
                   CONFIG.BSTYPE,
                   "rootbs",
                   CONFIG.BSARGS("noexist"))
    
  def test_create(self):    
    CONFIG.remove_bs(self.bspath)

    bs = utp.BlockStore.create(CONFIG.BSTYPE,
                               "rootbs",
                               CONFIG.BSSIZE,
                               CONFIG.BSARGS(self.bspath))
    bs.bs_close()

    CONFIG.remove_bs(self.bspath)
    
  def test_should_be_able_to_create_close_and_open_a_block_store(self):
    CONFIG.remove_bs(self.bspath)
    bs = utp.BlockStore.create(CONFIG.BSTYPE,
                               "rootbs",
                               CONFIG.BSSIZE,
                               CONFIG.BSARGS(self.bspath))
       
    k = buffer("persistentkey%(random.randrange(999999999))")
    v = buffer("persistentvalue")      
    bs.bs_put_block(k, v)    
    bs.bs_close()
    
    bs1 = utp.BlockStore.open(CONFIG.BSTYPE,
                              "rootbs",
                              CONFIG.BSARGS(self.bspath))
    b = bs1.bs_get_block(k)
    bs1.bs_close()
    CONFIG.remove_bs(self.bspath)
    
    assert b == v
  
  
  def test_create_on_prexisting_should_throw_error(self):
    CONFIG.remove_bs(self.bspath)

    bs = utp.BlockStore.create(CONFIG.BSTYPE,
                               "rootbs",
                               CONFIG.BSSIZE,
                               CONFIG.BSARGS(self.bspath))

    py.test.raises(utp.NotUniqueError,
                   utp.BlockStore.create,
                   CONFIG.BSTYPE,
                   "rootbs",
                   CONFIG.BSSIZE,
                   CONFIG.BSARGS(self.bspath))

    bs.bs_close()

    CONFIG.remove_bs(self.bspath)

