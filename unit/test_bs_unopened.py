import sys
import random
import py
import os
import utp
import utp.BlockStore

import CONFIG

class TestUnopenedBlockStore:

  def setup_class(self):
    self.bspath = "fs_unopened.bs"
    CONFIG.remove_bs(self.bspath)

  def teardown_class(self):
    pass
    
  def test_open_on_nonexistent(self):

    # BOGUS - We're having trouble matching exceptions.
    # Replace Exception w/ NotFoundError.
    #
    py.test.raises(Exception,
                   utp.BlockStore.open, CONFIG.BSTYPE, ("NOEXIST",))
    
  def test_create(self):    
    CONFIG.remove_bs(self.bspath)  

    bs = utp.BlockStore.create(CONFIG.BSTYPE, (self.bspath,))
    bs.bs_close()

    CONFIG.remove_bs(self.bspath)  
    
  def test_should_be_able_to_create_close_and_open_a_block_store(self):
    CONFIG.remove_bs(self.bspath)
    bs = utp.BlockStore.create(CONFIG.BSTYPE, (self.bspath,))
       
    k = buffer("persistentkey%(random.randrange(999999999))")
    v = buffer("persistentvalue")      
    bs.bs_put_block(k, v)    
    bs.bs_close()
    
    bs1 = utp.BlockStore.open(CONFIG.BSTYPE, (self.bspath,))
    b = bs1.bs_get_block(k)
    bs1.bs_close()
    CONFIG.remove_bs(self.bspath)
    
    assert b == v
  
  
  def test_create_on_prexisting_should_throw_error(self):
    CONFIG.remove_bs(self.bspath)  

    bs = utp.BlockStore.create(CONFIG.BSTYPE, (self.bspath,))

    # BOGUS - We're having trouble matching exceptions.
    # Replace Exception w/ NotUniqueError
    #
    py.test.raises(Exception,
                   utp.BlockStore.create, CONFIG.BSTYPE, (self.bspath,))
    bs.bs_close()

    CONFIG.remove_bs(self.bspath)  

