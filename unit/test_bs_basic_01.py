import sys
import random
import py
import utp
import utp.BlockStore

import CONFIG

class Test_bs_basic_01:

  def setup_class(self):
    self.bspath = "bs_basic_01"
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    CONFIG.BSSIZE,
                                    (self.bspath,))
    
  def teardown_class(self):
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)


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

    # BOGUS - We're having trouble matching exceptions.
    # Replace Exception w/ NotFoundError.
    #
    py.test.raises(Exception, "self.bs.bs_get_block(k)")
  
    
  def test_delete_block(self):
    k = buffer("thekey%(random.randrange(999999999))")
    v = buffer("thevalue%(random.randrange(999999999))")

    self.bs.bs_put_block(k, v)   
    self.bs.bs_del_block(k)
    
    # BOGUS - We're having trouble matching exceptions.
    # Replace Exception w/ NotFoundError.
    #
    py.test.raises(Exception, self.bs.bs_get_block,k)
    
  def test_second_put_with_existing_key_should_not_be_stored(self):
    k = buffer("test_second_put_with_existing_key_should_not_be_stored")
    v1 = buffer("value1")
    v2 = buffer("value2")

    self.bs.bs_put_block(k, v1)
    self.bs.bs_put_block(k, v2)    
    v = self.bs.bs_get_block(k)
    
    assert v == v1   
  
  	
