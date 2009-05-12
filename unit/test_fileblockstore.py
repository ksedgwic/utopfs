import sys
import random
import py

import utp
import utp.BlockStore

class TestBlockStore:

  def setup_class(self):
    # Find the BlockStore singleton.
    self.bs = utp.BlockStore.instance()
    
    # Create the BlockStore at a path.
    self.bs.bs_create("myfsbs")
    
  def teardown_class(self):
    self.bs.bs_close()

  def test_create_should_be_empty(self):
    k = buffer("test_for_delete")
    v = buffer("test_for_delete")
    self.bs.bs_put_block(k,v)
    self.bs.bs_close()
    self.bs.bs_create("myfsbs")
    
    py.test.raises(Exception,"self.bs.bs_get_block(k)")

  def test_put_and_get_block(self):    
        
    k = buffer("thekey%(random.randrange(999999999))")
    v = buffer("thevalue%(random.randrange(999999999))")

    # Put a block of data.
    self.bs.bs_put_block(k, v)
    
    # Get a block of data.
    b = self.bs.bs_get_block(k)
    assert v == b
   
   
  def test_get_block_that_does_not_exist(self):
    k = buffer("badkey%(random.randrange(999999999))")
    py.test.raises(Exception,"self.bs.bs_get_block(k)")
  
    
  def test_delete_block(self):
    k = buffer("thekey%(random.randrange(999999999))")
    v = buffer("thevalue%(random.randrange(999999999))")

    self.bs.bs_put_block(k, v)   
    self.bs.bs_del_block(k)
    
    py.test.raises(Exception,self.bs.bs_get_block,k)
    

    

