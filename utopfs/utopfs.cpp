// This is a silly comment

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include <ace/Service_Config.h>

#define FUSE_USE_VERSION 26

#include <fuse.h>

using namespace std;

extern "C" {

static char const * utopfs_dir_path = "/.utopfs";
static char const * utopfs_ver_path = "/.utopfs/version";
static char const * utopfs_ver_str = "utopfs version 0.1\n";

static int
utopfs_getattr(char const * i_path,
              struct stat * stbuf)
{
    int res = 0; /* temporary result */

    memset(stbuf, 0, sizeof(struct stat));

    string path = i_path;

    if (path == "/")
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if (path == utopfs_dir_path)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if (path == utopfs_ver_path)
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(utopfs_ver_str);
    }
    else
    {
        res = -ENOENT;
    }
    return res;
}

static int
utopfs_readdir(char const * i_path,
              void * buf,
              fuse_fill_dir_t filler,
              off_t offset,
              struct fuse_file_info * fi)
{
   (void) offset;
   (void) fi;

   string path = i_path;

   if (path == "/")
   {
       filler(buf, ".", NULL, 0);
       filler(buf, "..", NULL, 0);
       filler(buf, utopfs_dir_path + 1, NULL, 0);
   }
   else if (path == utopfs_dir_path)
   {
       filler(buf, ".", NULL, 0);
       filler(buf, "..", NULL, 0);
       filler(buf, utopfs_ver_path + 9, NULL, 0);
   }
   else
   {
       return -ENOENT;
   }

   return 0;
}

static int
utopfs_open(char const * i_path,
           struct fuse_file_info *fi)
{
   string path = i_path;

   if (path != utopfs_ver_path)
       return -ENOENT;

   if ((fi->flags & 3) != O_RDONLY )
       return -EACCES;

   return 0;
}

static int
utopfs_read(char const * i_path,
           char * buf,
           size_t size,
           off_t offset,
           struct fuse_file_info *fi)
{
   (void) fi;

   size_t len;

   string path = i_path;

   if (path != utopfs_ver_path)
       return -ENOENT;

   len = strlen(utopfs_ver_str);
   if (offset < off_t(len))
   {
      if (offset + size > len)
         size = len - offset;
      memcpy(buf, utopfs_ver_str + offset, size);
   }
   else
   {
       size = 0;
   }
   return size;
}

static struct fuse_operations utopfs_oper;

} // end extern "C"

int
main(int argc, char ** argv)
{
    ACE_Service_Config::open(argc, argv);

    utopfs_oper.getattr		= utopfs_getattr;
    utopfs_oper.readdir		= utopfs_readdir;
    utopfs_oper.open		= utopfs_open;
    utopfs_oper.read		= utopfs_read;
    
    return fuse_main(argc, argv, &utopfs_oper, NULL);
}
