import sys
import random
import py
import utp
import utp.BlockStore

import CONFIG

class Test_bs_basic_01:

  def setup_class(self):
    self.bspath = "bs_basic_01"
    CONFIG.unmap_bs("rootbs")
    CONFIG.remove_bs(self.bspath)
    self.bs = utp.BlockStore.create(CONFIG.BSTYPE,
                                    "rootbs",
                                    CONFIG.BSSIZE,
                                    CONFIG.BSARGS(self.bspath))
    
  def teardown_class(self):
    self.bs.bs_close()
    CONFIG.remove_bs(self.bspath)

  def test_block_put_and_get(self):    
        
    k = buffer("thekey%(random.randrange(999999999))")
    v = buffer("thevalue%(random.randrange(999999999))")
    
	
    # Put a block of data.
    self.bs.bs_block_put(k, v)
    
    # Get a block of data.
    b = self.bs.bs_block_get(k)
    assert v == b
   
   
  def test_block_get_that_does_not_exist(self):
    k = buffer("badkey%(random.randrange(999999999))")
    py.test.raises(utp.NotFoundError, self.bs.bs_block_get, k)
