import os
import pwd
import grp
import shutil
import string

import utp

# Who shall we test as?
UNAME = pwd.getpwuid(os.geteuid()).pw_name
GNAME = grp.getgrgid(os.getegid()).gr_name

# Which BlockStore module should we use?

## # BDBBlockStore
## #
## BSTYPE = "BDBBS"
## BSSIZE = 1 * 1024 * 1024 * 1024
## def BSARGS(bspath): return (bspath,)

## # FSBlockStore
## #
## BSTYPE = "FSBS"
## BSSIZE = 1 * 1024 * 1024 * 1024
## def BSARGS(bspath): return (bspath,)

# S3BlockStore
#
key_id = os.environ['S3_ACCESS_KEY_ID']
secret = os.environ['S3_SECRET_ACCESS_KEY']
BSTYPE = "S3BS"
BSSIZE = 1 * 1024 * 1024 * 1024
def BSARGS(bspath):
  # S3 doesn't allow underscores.
  bspath = string.replace(bspath, "_", "-")
  return ("--s3-access-key-id=%s" % key_id,
          "--s3-secret-access-key=%s" % secret,
          "--bucket=ksedgwic-utopfs-unit-%s" % bspath)

# Which FileSystem module should we use?
FSTYPE = "UTFS"
FSARGS = ()

def remove_bs(path):
  try:
    utp.BlockStore.destroy(BSTYPE, BSARGS(path))
  except utp.NotFoundError, ex:
    # It's ok of the blockstore doesn't exist ..
    pass
