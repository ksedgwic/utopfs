import os
import pwd
import grp
import shutil

# Who shall we test as?
UNAME = pwd.getpwuid(os.geteuid()).pw_name
GNAME = grp.getgrgid(os.getegid()).gr_name

# Which BlockStore module should we use?
BSTYPE = "BDBBS"
BSARGS = ()

def remove_bs(path):
  try:
    os.unlink(path)
  except:
  	pass 

#def remove_bs(path):
#  shutil.rmtree(self.bspath,True)


# Which FileSystem module should we use?
FSTYPE = "UTFS"
FSARGS = ()

