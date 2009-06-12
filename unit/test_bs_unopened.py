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
    # If you open an non-existent blockstore you should get
    # a NotFound Exception.
    py.test.raises(utp.NotFoundError,
                   utp.BlockStore.open, CONFIG.BSTYPE, ("NOEXIST",))
    
  def test_create(self):    
    shutil.rmtree(self.bspath,True)  

    bs = utp.BlockStore.create(CONFIG.BSTYPE, (self.bspath,))
    bs.bs_close()

    shutil.rmtree(self.bspath,True)  
  
  def test_create_on_prexisting_should_throw_error(self):
    shutil.rmtree(self.bspath,True)  

    bs = utp.BlockStore.create(CONFIG.BSTYPE, (self.bspath,))

    # This is bogus, can't make NotUniqueError work here!
    py.test.raises(Exception,
                   utp.BlockStore.create, CONFIG.BSTYPE, (self.bspath,))
    bs.bs_close()

    shutil.rmtree(self.bspath,True)  

