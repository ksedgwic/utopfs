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

