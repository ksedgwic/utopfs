import sys
import shutil
import random
import py
import os
import utp
import utp.BlockStore


class TestUnopenedBlockStore:

  def setup_class(self):
    self.bspath = "fs_unopened.bs"
    shutil.rmtree(self.bspath,True)

    # Find the BlockStore singleton.
    self.bs = utp.BlockStore.instance()
    

  def teardown_class(self):
    self.bs.bs_close()    
    
  def test_open_on_nonexistent(self):
    # If you open an non-existent blockstore you should get
    # a NotFound Exception.
    py.test.raises(Exception,"self.bs.bs_open(\"NOEXIST\")")
    
  def test_create(self):    
    shutil.rmtree(self.bspath,True)  
    #  self.remove_blockstore()
    self.bs.bs_create(self.bspath)  
    self.bs.bs_close()
  
  def test_create_on_prexisting_should_throw_error(self):
    shutil.rmtree(self.bspath,True)  
    self.bs.bs_create(self.bspath)
    py.test.raises(Exception,"self.bs.bs_create(self.bspath)")
    
    
    

