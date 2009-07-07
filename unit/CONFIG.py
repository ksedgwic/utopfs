import os
import pwd
import grp
import shutil

# Who shall we test as?
UNAME = pwd.getpwuid(os.geteuid()).pw_name
GNAME = grp.getgrgid(os.getegid()).gr_name

# Which BlockStore module should we use?

# BDBBlockStore
#
BSTYPE = "BDBBS"
BSSIZE = 1 * 1024 * 1024 * 1024
BSARGS = ()

def remove_bs(path):
  shutil.rmtree(path, True)

# use this for files:
#def remove_bs(path):
#  try:
#    os.unlink(path)
#  except:
#    pass

# FSBlockStore
#
#BSTYPE = "FSBS"
#BSSIZE = 1 * 1024 * 1024 * 1024
#BSARGS = ()

#def remove_bs(path):
#  shutil.rmtree(path, True)


# Which FileSystem module should we use?
FSTYPE = "UTFS"
FSARGS = ()

