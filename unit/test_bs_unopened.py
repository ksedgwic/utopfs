import sys
import random
import py

import utp
import utp.BlockStore

class TestBlockStore:

  def setup_class(self):
    # Find the BlockStore singleton.
    self.bs = utp.BlockStore.instance()
    
  def teardown_class(self):
    self.bs.bs_close()

  def test_open_on_nonexistent(self):
    # If you open an non-existent blockstore you should get
    # a NotFound Exception.
    py.test.raises(Exception,"self.bs.bs_open(\"NOEXIST\")")
