#include <sstream>
#include <string>

#include "Log.h"
#include "BlockStore.h"

#include "UTFileSystem.h"
#include "utfslog.h"

#include "Base32.h"

using namespace std;
using namespace utp;

namespace UTFS {

static char const * utopfs_dir_path = "/.utopfs";
static char const * utopfs_ver_path = "/.utopfs/version";
static char const * utopfs_ver_str = "utopfs version 0.1\n";

UTFileSystem::UTFileSystem()
{
    LOG(lgr, 4, "CTOR");
}

UTFileSystem::~UTFileSystem()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

void
UTFileSystem::fs_mkfs(std::string const & i_path)
    throw (utp::InternalError,
           utp::ValueError)
{
    m_bsh = BlockStore::instance();
    m_bsh->bs_create(i_path);
}

void
UTFileSystem::fs_mount(std::string const & i_path)
    throw (utp::InternalError,
           utp::ValueError)
{
    m_bsh = BlockStore::instance();
    m_bsh->bs_open(i_path);
}

int
UTFileSystem::fs_getattr(string const & i_path,
                         struct stat * o_stbuf)
    throw (utp::InternalError)
{
    int res = 0; /* temporary result */

    memset(o_stbuf, 0, sizeof(struct stat));

    if (i_path == "/")
    {
        o_stbuf->st_mode = S_IFDIR | 0755;
        o_stbuf->st_nlink = 2;
    }
    else if (i_path == utopfs_dir_path)
    {
        o_stbuf->st_mode = S_IFDIR | 0755;
        o_stbuf->st_nlink = 2;
    }
    else if (i_path == utopfs_ver_path)
    {
        o_stbuf->st_mode = S_IFREG | 0444;
        o_stbuf->st_nlink = 1;
        o_stbuf->st_size = strlen(utopfs_ver_str);
    }
    else
    {
        res = -ENOENT;
    }
    return res;
}

int
UTFileSystem::fs_open(string const & i_path,
                      int i_flags)
    throw (utp::InternalError)
{
   if (i_path != utopfs_ver_path)
       return -ENOENT;

   if ((i_flags & 3) != O_RDONLY )
       return -EACCES;

   return 0;
}

int
UTFileSystem::fs_read(string const & i_path,
                      void * o_bufptr,
                      size_t i_size,
                      off_t i_off)
    throw (utp::InternalError)
{
   if (i_path != utopfs_ver_path)
       return -ENOENT;

   off_t len = strlen(utopfs_ver_str);
   if (i_off < len)
   {
       if (i_off + off_t(i_size) > len)
          i_size = len - i_off;
      memcpy(o_bufptr, utopfs_ver_str + i_off, i_size);
   }
   else
   {
       i_size = 0;
   }
   return int(i_size);
}

int
UTFileSystem::fs_readdir(string const & i_path,
                         off_t i_offset,
                         DirEntryFunc & o_entryfunc)
    throw (utp::InternalError)
{
   if (i_path == "/")
   {
       o_entryfunc(".", NULL, 0);
       o_entryfunc("..", NULL, 0);
       o_entryfunc(utopfs_dir_path + 1, NULL, 0);
   }
   else if (i_path == utopfs_dir_path)
   {
       o_entryfunc(".", NULL, 0);
       o_entryfunc("..", NULL, 0);
       o_entryfunc(utopfs_ver_path + 9, NULL, 0);
   }
   else
   {
       return -ENOENT;
   }

   return 0;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
