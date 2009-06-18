// This is a silly comment

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include <ace/Service_Config.h>

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <fuse/fuse_opt.h>

#include "BlockStoreFactory.h"
#include "Except.h"
#include "FileSystemFactory.h"
#include "FileSystem.h"
#include "Log.h"

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
    int loglevel;
    string path;
    string fsid;
    string passphrase;
    bool do_mkfs;
    FileSystemHandle fsh;
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
        return utopfs.fsh->fs_getattr(i_path, stbuf);
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
        return utopfs.fsh->fs_readlink(i_path, o_obuf, i_size);
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
        return utopfs.fsh->fs_mknod(i_path, i_mode, i_dev);
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
        return utopfs.fsh->fs_mkdir(i_path, i_mode);
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
        return utopfs.fsh->fs_unlink(i_path);
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
        return utopfs.fsh->fs_rmdir(i_path);
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
        return utopfs.fsh->fs_symlink(i_opath, i_npath);
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
        return utopfs.fsh->fs_rename(i_opath, i_npath);
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }
}

static int
utopfs_link(char const * i_opath, char const * i_npath)
{
    try
    {
        return utopfs.fsh->fs_link(i_opath, i_npath);
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
        return utopfs.fsh->fs_chmod(i_path, i_mode);
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
        return utopfs.fsh->fs_truncate(i_path, i_size);
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
        return utopfs.fsh->fs_open(i_path, fi->flags);
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
        return utopfs.fsh->fs_read(i_path, buf, size, offset);
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
        return utopfs.fsh->fs_write(i_path, buf, size, offset);
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
        return utopfs.fsh->fs_readdir(i_path, offset, mdef);
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
        return utopfs.fsh->fs_access(i_path, i_mode);
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
        return utopfs.fsh->fs_utime(i_path, i_tv[0], i_tv[1]);
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
    KEY_LOGLEVEL,
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
	FUSE_OPT_KEY("-L",             KEY_LOGLEVEL),
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
    case KEY_LOGLEVEL:
        utopfs.loglevel = atoi(arg);
        return 0;

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

#define LCLCONF "./utopfs.conf"
#define SYSCONF "/etc/sysconfig/utopfs/utopfs.conf"
void
init_modules(string const & argv0)
{
    ACE_stat statbuf;

    // Build a synthetic command line.
    vector<string> args;
    args.push_back(argv0);

	// Enable ACE debugging
    // args.push_back("-d");

    // Is there a UTOPFS_SVCCONF env variable?
    char * svcconfenv = ACE_OS::getenv("UTOPFS_SVCCONF");
    if (svcconfenv && *svcconfenv)
    {
        args.push_back("-f");
        args.push_back(svcconfenv);
    }

    // Is there a local utopfs.conf file?
    else if (ACE_OS::stat(LCLCONF, &statbuf) == 0)
    {
        args.push_back("-f");
        args.push_back(LCLCONF);
    }

    // Is there a system utopfs.conf file?
    else if (ACE_OS::stat(SYSCONF, &statbuf) == 0)
    {
        args.push_back("-f");
        args.push_back(SYSCONF);
    }

    // Convert to int, char**
    char ** argv = (char **) malloc(sizeof(char *) * (args.size() + 1));
    for (unsigned i = 0; i < args.size(); ++i)
        argv[i] = strdup(args[i].c_str());
    argv[args.size()] = NULL; // sentinal

    // Initialize the modules.
    int rv = ACE_Service_Config::open(args.size(), argv);
    if (rv == -1)
        throwstream(OperationError,
                    "service config failed: " << ACE_OS::strerror(errno));
    else if (rv > 0)
        throwstream(OperationError,
                    rv << " errors while parsing service config");
}

int
main(int argc, char ** argv)
{
    // Setup defaults
    utopfs.loglevel = -1;
    utopfs.do_mkfs = false;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, &utopfs, utopfs_opts, utopfs_opt_proc) == -1)
        fatal("option parsing failed");

    // When we are daemonized our path is changed to '/'.  If the
    // blockstore path is relative convert it to absolute ...
    //
    if (utopfs.path[0] != '/')
    {
        char cwdbuf[MAXPATHLEN];
        string cwd = getcwd(cwdbuf, sizeof(cwdbuf));
        utopfs.path = cwd + '/' + utopfs.path;
    }

    utopfs_oper.getattr		= utopfs_getattr;
    utopfs_oper.readlink	= utopfs_readlink;
    utopfs_oper.mknod		= utopfs_mknod;
    utopfs_oper.mkdir		= utopfs_mkdir;
    utopfs_oper.unlink		= utopfs_unlink;
    utopfs_oper.rmdir		= utopfs_rmdir;
    utopfs_oper.symlink		= utopfs_symlink;
    utopfs_oper.rename		= utopfs_rename;
    utopfs_oper.link		= utopfs_link;
    utopfs_oper.chmod		= utopfs_chmod;
    utopfs_oper.truncate	= utopfs_truncate;
    utopfs_oper.open		= utopfs_open;
    utopfs_oper.read		= utopfs_read;
    utopfs_oper.write		= utopfs_write;
    utopfs_oper.readdir		= utopfs_readdir;
    utopfs_oper.access		= utopfs_access;
    utopfs_oper.utimens		= utopfs_utimens;

    /// Modules (including logging) load and start here.
    init_modules(argv[0]);

    /// If the logging was specified, set it.
    if (utopfs.loglevel != -1)
        theRootLogCategory.logger_level(utopfs.loglevel);

    // Perform the mount.
    try
    {
        if (utopfs.do_mkfs)
        {
            StringSeq bsargs;
            bsargs.push_back(utopfs.path);
            BlockStoreHandle bsh = BlockStoreFactory::create("FSBS", bsargs);

            StringSeq fsargs;
            utopfs.fsh = FileSystemFactory::mkfs("UTFS",
                                                 bsh,
                                                 utopfs.fsid,
                                                 utopfs.passphrase,
                                                 fsargs);
        }
        else
        {
            StringSeq bsargs;
            bsargs.push_back(utopfs.path);
            BlockStoreHandle bsh = BlockStoreFactory::open("FSBS", bsargs);

            StringSeq fsargs;
            utopfs.fsh = FileSystemFactory::mount("UTFS",
                                                  bsh,
                                                  utopfs.fsid,
                                                  utopfs.passphrase,
                                                  fsargs);
        }
    }
    catch (utp::Exception const & ex)
    {
        return fatal(ex.what());
    }

    return fuse_main(args.argc, args.argv, &utopfs_oper, NULL);
}
