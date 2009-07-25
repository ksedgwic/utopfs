import os
import pwd
import grp
import shutil

import utp

# Who shall we test as?
UNAME = pwd.getpwuid(os.geteuid()).pw_name
GNAME = grp.getgrgid(os.getegid()).gr_name

# Which BlockStore module should we use?

## # BDBBlockStore
## #
## BSTYPE = "BDBBS"
## BSSIZE = 1 * 1024 * 1024 * 1024
## BSARGS = ()

# FSBlockStore

BSTYPE = "FSBS"
BSSIZE = 1 * 1024 * 1024 * 1024
BSARGS = ()

# Which FileSystem module should we use?
FSTYPE = "UTFS"
FSARGS = ()

def remove_bs(path):
  try:
    utp.BlockStore.destroy(BSTYPE, (path,))  
  except utp.NotFoundError, ex:
    # It's ok of the blockstore doesn't exist ..
    pass
