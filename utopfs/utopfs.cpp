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

#include "Except.h"
#include "FileSystem.h"

using namespace std;
using namespace utp;

extern "C" {

static int fatal(char const * msg)
{
    // FIXME - Is this the best way to deal w/ these errors?
    cerr << msg;
    abort();
    return 0;	// makes compiler happy
}        

static int
utopfs_getattr(char const * i_path,
               struct stat * stbuf)
{
    try
    {
        return FileSystem::instance()->fs_getattr(i_path, stbuf);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

struct MyDirEntryFunc : public utp::FileSystem::DirEntryFunc
{
    void *				m_buf;
    fuse_fill_dir_t		m_filler;

    MyDirEntryFunc(void * i_buf, fuse_fill_dir_t i_filler)
        : m_buf(i_buf), m_filler(i_filler) {}

    virtual bool operator()(string const & i_name,
                            struct stat const * i_stbuf,
                            off_t i_off)
    {
        return m_filler(m_buf, i_name.c_str(), i_stbuf, i_off) != 0;
    }
};

static int
utopfs_readdir(char const * i_path,
               void * buf,
               fuse_fill_dir_t filler,
               off_t offset,
               struct fuse_file_info * fi)
{
    try
    {
        MyDirEntryFunc mdef(buf, filler);
        return FileSystem::instance()->fs_readdir(i_path, offset, mdef);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_open(char const * i_path,
            struct fuse_file_info *fi)
{
    try
    {
        return FileSystem::instance()->fs_open(i_path, fi->flags);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_read(char const * i_path,
            char * buf,
            size_t size,
            off_t offset,
            struct fuse_file_info *fi)
{
    try
    {
        return FileSystem::instance()->fs_read(i_path, buf, size, offset);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
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
