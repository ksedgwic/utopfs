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
#include <fuse/fuse_opt.h>

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

struct utopfs
{
    struct fuse_args utop_args;
    string path;
    string fsid;
    string passphrase;
    bool do_mkfs;
};

static struct utopfs utopfs;

struct MyDirEntryFunc : public utp::FileSystem::DirEntryFunc
{
    void *				m_buf;
    fuse_fill_dir_t		m_filler;

    MyDirEntryFunc(void * i_buf, fuse_fill_dir_t i_filler)
        : m_buf(i_buf), m_filler(i_filler) {}

    virtual bool def_entry(string const & i_name,
                           struct stat const * i_stbuf,
                           off_t i_off)
    {
        return m_filler(m_buf, i_name.c_str(), i_stbuf, i_off) != 0;
    }
};

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

static int
utopfs_readlink(char const * i_path,
                char * o_obuf,
                size_t i_size)
{
    try
    {
        return FileSystem::instance()->fs_readlink(i_path, o_obuf, i_size);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_mknod(char const * i_path, mode_t i_mode, dev_t i_dev)
{
    try
    {
        return FileSystem::instance()->fs_mknod(i_path, i_mode, i_dev);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_mkdir(char const * i_path, mode_t i_mode)
{
    try
    {
        return FileSystem::instance()->fs_mkdir(i_path, i_mode);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_unlink(char const * i_path)
{
    try
    {
        return FileSystem::instance()->fs_unlink(i_path);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_rmdir(char const * i_path)
{
    try
    {
        return FileSystem::instance()->fs_rmdir(i_path);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_symlink(char const * i_opath, char const * i_npath)
{
    try
    {
        return FileSystem::instance()->fs_symlink(i_opath, i_npath);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_rename(char const * i_opath, char const * i_npath)
{
    try
    {
        return FileSystem::instance()->fs_rename(i_opath, i_npath);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_chmod(char const * i_path, mode_t i_mode)
{
    try
    {
        return FileSystem::instance()->fs_chmod(i_path, i_mode);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_truncate(char const * i_path, off_t i_size)
{
    try
    {
        return FileSystem::instance()->fs_truncate(i_path, i_size);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_open(char const * i_path, struct fuse_file_info *fi)
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

static int
utopfs_write(char const * i_path,
             char const * buf,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
    try
    {
        return FileSystem::instance()->fs_write(i_path, buf, size, offset);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

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
utopfs_access(char const * i_path,
              int i_mode)
{
    try
    {
        return FileSystem::instance()->fs_access(i_path, i_mode);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_utimens(char const * i_path, struct timespec const i_tv[2])
{
    try
    {
        return FileSystem::instance()->fs_utime(i_path, i_tv[0], i_tv[1]);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static struct fuse_operations utopfs_oper;

} // end extern "C"

static const char *utop_opts[] = {
	NULL,
};

enum {
    KEY_MKFS,
    KEY_FSID,
    KEY_PASSPHRASE,
	KEY_HELP,
	KEY_VERSION,
	KEY_FOREGROUND,
	KEY_CONFIGFILE,
};

#define CPP_FUSE_OPT_END	{ NULL, 0, 0 }

static struct fuse_opt utopfs_opts[] = {
	FUSE_OPT_KEY("-M",             KEY_MKFS),
	FUSE_OPT_KEY("-F ",            KEY_FSID),
	FUSE_OPT_KEY("-P ",            KEY_PASSPHRASE),
	FUSE_OPT_KEY("-V",             KEY_VERSION),
	FUSE_OPT_KEY("--version",      KEY_VERSION),
	FUSE_OPT_KEY("-h",             KEY_HELP),
	FUSE_OPT_KEY("--help",         KEY_HELP),
	FUSE_OPT_KEY("debug",          KEY_FOREGROUND),
	FUSE_OPT_KEY("-d",             KEY_FOREGROUND),
	FUSE_OPT_KEY("-f",             KEY_FOREGROUND),
	CPP_FUSE_OPT_END
};

static int is_utop_opt(const char *arg)
{
	if (arg[0] != '-') {
		unsigned arglen = strlen(arg);
		const char **o;
		for (o = utop_opts; *o; o++) {
			unsigned olen = strlen(*o);
			if (arglen > olen && arg[olen] == '=' &&
			    strncasecmp(arg, *o, olen) == 0)
				return 1;
		}
	}
	return 0;
}

static void utop_add_arg(char const * arg)
{
	if (fuse_opt_add_arg(&utopfs.utop_args, arg) == -1)
		_exit(1);
}

static int utopfs_opt_proc(void * data,
                           char const * arg,
                           int key,
                           struct fuse_args * outargs)
{
	(void) data;

	switch (key)
    {
    case KEY_MKFS:
        utopfs.do_mkfs = true;
        return 0;

    case KEY_FSID:
        utopfs.fsid = arg;
        return 0;

    case KEY_PASSPHRASE:
        utopfs.passphrase = arg;
        return 0;

	case FUSE_OPT_KEY_OPT:
		if (is_utop_opt(arg))
        {
            ostringstream tmpstrm;
            tmpstrm << "-o" << arg;
			utop_add_arg(tmpstrm.str().c_str());
			return 0;
		}
		return 1;

	case FUSE_OPT_KEY_NONOPT:
        if (utopfs.path.empty())
        {
            utopfs.path = arg;
            return 0;
        }
        return 1;

	case KEY_HELP:
	case KEY_VERSION:
	case KEY_FOREGROUND:
	case KEY_CONFIGFILE:
		return 1;

	default:
		fprintf(stderr, "internal error\n");
		abort();
	}
}

int
main(int argc, char ** argv)
{
    utopfs.do_mkfs = false;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, &utopfs, utopfs_opts, utopfs_opt_proc) == -1)
        fatal("option parsing failed");

    utopfs_oper.getattr		= utopfs_getattr;
    utopfs_oper.readlink	= utopfs_readlink;
    utopfs_oper.mknod		= utopfs_mknod;
    utopfs_oper.mkdir		= utopfs_mkdir;
    utopfs_oper.unlink		= utopfs_unlink;
    utopfs_oper.rmdir		= utopfs_rmdir;
    utopfs_oper.symlink		= utopfs_symlink;
    utopfs_oper.rename		= utopfs_rename;
    utopfs_oper.chmod		= utopfs_chmod;
    utopfs_oper.truncate	= utopfs_truncate;
    utopfs_oper.open		= utopfs_open;
    utopfs_oper.read		= utopfs_read;
    utopfs_oper.write		= utopfs_write;
    utopfs_oper.readdir		= utopfs_readdir;
    utopfs_oper.access		= utopfs_access;
    utopfs_oper.utimens		= utopfs_utimens;

    ACE_Service_Config::open(argv[0]);

    // Perform the mount.
    try
    {
        if (utopfs.do_mkfs)
            FileSystem::instance()->fs_mkfs(utopfs.path,
                                            utopfs.fsid,
                                            utopfs.passphrase);
        else
            FileSystem::instance()->fs_mount(utopfs.path,
                                             utopfs.fsid,
                                             utopfs.passphrase);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }

    return fuse_main(args.argc, args.argv, &utopfs_oper, NULL);
}
