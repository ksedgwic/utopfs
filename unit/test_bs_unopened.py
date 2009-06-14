import sys
import shutil
import random
import py
import os
import utp
import utp.BlockStore

import CONFIG

class TestUnopenedBlockStore:

  def setup_class(self):
    self.bspath = "fs_unopened.bs"
    shutil.rmtree(self.bspath,True)

  def teardown_class(self):
    pass
    
  def test_open_on_nonexistent(self):

    # BOGUS - We're having trouble matching exceptions.
    # Replace Exception w/ NotFoundError.
    #
    py.test.raises(Exception,
                   utp.BlockStore.open, CONFIG.BSTYPE, ("NOEXIST",))
    
  def test_create(self):    
    shutil.rmtree(self.bspath,True)  

    bs = utp.BlockStore.create(CONFIG.BSTYPE, (self.bspath,))
    bs.bs_close()

    shutil.rmtree(self.bspath,True)  
  
  def test_create_on_prexisting_should_throw_error(self):
    shutil.rmtree(self.bspath,True)  

    bs = utp.BlockStore.create(CONFIG.BSTYPE, (self.bspath,))

    # BOGUS - We're having trouble matching exceptions.
    # Replace Exception w/ NotUniqueError
    #
    py.test.raises(Exception,
                   utp.BlockStore.create, CONFIG.BSTYPE, (self.bspath,))
    bs.bs_close()

    shutil.rmtree(self.bspath,True)  

