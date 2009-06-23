import os
import pwd
import grp

# Who shall we test as?
UNAME = pwd.getpwuid(os.geteuid()).pw_name
GNAME = grp.getgrgid(os.getegid()).gr_name

# Which BlockStore module should we use?
BSTYPE = "FSBS"
BSARGS = ()

# Which FileSystem module should we use?
FSTYPE = "UTFS"
FSARGS = ()
