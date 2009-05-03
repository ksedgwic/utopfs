#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUSE_USE_VERSION 26

#include <fuse.h>

#include <iostream>

using namespace std;

static char const * hello_str = "Hello World!\n";
static char const * hello_path = "/hello";

extern "C" {

static int
hello_getattr(char const * path,
              struct stat * stbuf)
{
    int res = 0; /* temporary result */

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if (strcmp(path, hello_path) == 0)
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(hello_str);
    }
    else
    {
        res = -ENOENT;
    }
    return res;
}

static int
hello_readdir(char const * path,
              void * buf,
              fuse_fill_dir_t filler,
              off_t offset,
              struct fuse_file_info * fi)
{
   (void) offset;
   (void) fi;

   if (strcmp(path, "/") != 0)
       return -ENOENT;

   filler(buf, ".", NULL, 0);
   filler(buf, "..", NULL, 0);
   filler(buf, hello_path + 1, NULL, 0);

   return 0;
}

static int
hello_open(char const * path,
           struct fuse_file_info *fi)
{
   if (strcmp(path, hello_path) != 0)
       return -ENOENT;
   if ((fi->flags & 3) != O_RDONLY )
       return -EACCES;
   return 0;
}

static int
hello_read(char const * path,
           char * buf,
           size_t size,
           off_t offset,
           struct fuse_file_info *fi)
{
   (void) fi;

   size_t len;

   if (strcmp(path, hello_path) != 0)
       return -ENOENT;

   len = strlen(hello_str);
   if (offset < off_t(len))
   {
      if (offset + size > len)
         size = len - offset;
      memcpy(buf, hello_str + offset, size);
   }
   else
   {
       size = 0;
   }
   return size;
}

static struct fuse_operations hello_oper;

} // end extern "C"

int
main(int argc, char ** argv)
{
    hello_oper.getattr	= hello_getattr;
    hello_oper.readdir	= hello_readdir;
    hello_oper.open		= hello_open;
    hello_oper.read		= hello_read;
    
    return fuse_main(argc, argv, &hello_oper, NULL);
}
