#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <cassert>

#include <ace/Reactor.h>
#include <ace/Service_Config.h>
#include <ace/Task.h>
#include <ace/TP_Reactor.h>

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <fuse/fuse_opt.h>

#include "BlockStoreFactory.h"
#include "Controller.h"
#include "Except.h"
#include "FileSystemFactory.h"
#include "FileSystem.h"
#include "Log.h"

#include "fuselog.h"

using namespace std;
using namespace utp;

namespace {

string mapuid(uid_t uid)
{
    struct passwd pw;
    struct passwd * pwp;
    char buf[1024];
    int rv = getpwuid_r(uid, &pw, buf, sizeof(buf), &pwp);
    if (rv)
        throwstream(InternalError, FILELINE
                    << "getpwuid_r failed: " << strerror(rv));
    if (!pwp)
        throwstream(InternalError, FILELINE << "no password entry found");

    return pwp->pw_name;
}

string mapgid(gid_t gid)
{
    struct group gr;
    struct group * grp;
    char buf[1024];
    int rv = getgrgid_r(gid, &gr, buf, sizeof(buf), &grp);
    if (rv)
        throwstream(InternalError, FILELINE
                    << "getgrgid_r failed: " << strerror(rv));
    if (!grp)
        throwstream(InternalError, FILELINE << "no group entry found");

    return grp->gr_name;
}

class ThreadPool : public ACE_Task_Base
{
public:
    ThreadPool(ACE_Reactor * i_reactor, int i_numthreads)
        : m_reactor(i_reactor)
    {
        LOG(lgr, 4, "ThreadPool CTOR");

        if (activate(THR_NEW_LWP | THR_JOINABLE, i_numthreads) != 0)
            abort();
    }
    
    ~ThreadPool()
    {
        LOG(lgr, 4, "ThreadPool DTOR");
        wait();
    }

    virtual int svc(void)
    {
        LOG(lgr, 4, "ThreadPool starting");

        // Run the Reactor event loop.  Catch exceptions and report but keep
        // the threads running ...
        //
        while (true)
        {
            try
            {
                m_reactor->run_reactor_event_loop();
            }

            catch (exception const & ex)
            {
                cerr << "caught std::exception: " << ex.what() << endl;
            }
            catch (...)
            {
                cerr << "caught UNKNOWN EXCEPTION" << endl;
            }
        }

        LOG(lgr, 4, "ThreadPool finished");
        return 0;
    }

private:
    ACE_Reactor *		m_reactor;
};

} // end namespace

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
    string argv0;
    bool foreground;
    string lclconf;
    string bsid;
    int loglevel;
    string logpath;
    string path;
    string fsid;
    string passphrase;
    double syncsecs;
    uint64_t size;
    string mntpath;
    BlockStoreHandle bsh;
    FileSystemHandle fsh;
    Controller * control;
    ThreadPool * thrpool;
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

#define LCLCONF "./utopfs.conf"
#define SYSCONF "/etc/sysconfig/utopfs/utopfs.conf"
static void
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
    else if (ACE_OS::stat(utopfs.lclconf.c_str(), &statbuf) == 0)
    {
        args.push_back("-f");
        args.push_back(utopfs.lclconf);
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

static void *
utopfs_init(struct fuse_conn_info * i_conn)
{
    // NOTE - We wish we could do a lot of this stuff in the mainline
    // but much of it needs to be done after we daemonize ...

    struct fuse_context * fctxtp = fuse_get_context();

    // Set the log path and log level via env variables.
    ostringstream ostrm;
    ostrm << utopfs.loglevel;
    ACE_OS::setenv("UTOPFS_LOG_FILELEVEL", ostrm.str().c_str(), 1);
    ACE_OS::setenv("UTOPFS_LOG_FILEPATH", utopfs.logpath.c_str(), 1);

    if (!utopfs.foreground)
    {
        // Open stdin from /dev/null, stdout and stderr on the logfile.
        FILE * fp;
        fp = freopen("/dev/null", "rb", stdin);
        assert(fp && fileno(fp) == 0);
        fp = freopen(utopfs.logpath.c_str(), "ab", stdout);
        assert(fp && fileno(fp) == 1);
        fp = freopen(utopfs.logpath.c_str(), "ab", stderr);
        assert(fp && fileno(fp) == 2);
    }
    
    // Modules (including logging) load and start here.
    init_modules(utopfs.argv0);

    // Instantiate the reactor explicitly.
    ACE_Reactor::instance(new ACE_Reactor(new ACE_TP_Reactor), 1);

    // Start the thread pool.
    utopfs.thrpool = new ThreadPool(ACE_Reactor::instance(), 1);

    // Perform the mount.
    try
    {
        if (utopfs.size)
        {
            StringSeq bsargs;
            bsargs.push_back(utopfs.path);
            utopfs.bsh = BlockStoreFactory::create(utopfs.bsid,
                                                   utopfs.size,
                                                   bsargs);

            StringSeq fsargs;
            utopfs.fsh = FileSystemFactory::mkfs("UTFS",
                                                 utopfs.bsh,
                                                 utopfs.fsid,
                                                 utopfs.passphrase,
                                                 mapuid(fctxtp->uid),
                                                 mapgid(fctxtp->gid),
                                                 fsargs);
        }
        else
        {
            StringSeq bsargs;
            bsargs.push_back(utopfs.path);
            utopfs.bsh = BlockStoreFactory::open(utopfs.bsid, bsargs);

            StringSeq fsargs;
            utopfs.fsh = FileSystemFactory::mount("UTFS",
                                                  utopfs.bsh,
                                                  utopfs.fsid,
                                                  utopfs.passphrase,
                                                  fsargs);
        }

        // Start the controller.
        string controlpath = utopfs.mntpath + "/.utopfs/control";
        utopfs.control = new Controller(utopfs.bsh,
                                        utopfs.fsh,
                                        controlpath,
                                        utopfs.syncsecs);
        utopfs.control->init();
    }
    catch (utp::Exception const & ex)
    {
        fatal(ex.what());
    }

    return NULL;
}

static void
utopfs_destroy(void * ptr)
{
    try
    {
        // Flush the filesystem to the blockstore.
        utopfs.fsh->fs_sync();

        // Cleanup the control interface.
        utopfs.control->term();
    }
    catch (std::exception const & ex)
    {
        LOG(lgr, 1, "exception in utopfs_destroy: " << ex.what());
    }
}

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
        struct fuse_context * fctxtp = fuse_get_context();

        return utopfs.fsh->fs_mknod(i_path,
                                    i_mode,
                                    i_dev,
                                    mapuid(fctxtp->uid),
                                    mapgid(fctxtp->gid));
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
        struct fuse_context * fctxtp = fuse_get_context();

        return utopfs.fsh->fs_mkdir(i_path,
                                    i_mode,
                                    mapuid(fctxtp->uid),
                                    mapgid(fctxtp->gid));
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
             struct fuse_file_info * fi)
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
utopfs_statfs(char const * i_path,
              struct statvfs * statptr)
{
    try
    {
        return utopfs.fsh->fs_statfs(statptr);
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
    KEY_BSID,
    KEY_LOGLEVEL,
    KEY_LOGPATH,
    KEY_MKFS,
    KEY_FSID,
    KEY_PASSPHRASE,
	KEY_HELP,
	KEY_VERSION,
	KEY_SYNCSECS,
	KEY_FOREGROUND,
	KEY_CONFIGFILE,
};

#define CPP_FUSE_OPT_END	{ NULL, 0, 0 }

static struct fuse_opt utopfs_opts[] = {
	FUSE_OPT_KEY("-B ",            KEY_BSID),
	FUSE_OPT_KEY("-l ",            KEY_LOGLEVEL),
	FUSE_OPT_KEY("-L ",            KEY_LOGPATH),
	FUSE_OPT_KEY("-M ",            KEY_MKFS),
	FUSE_OPT_KEY("-F ",            KEY_FSID),
	FUSE_OPT_KEY("-P ",            KEY_PASSPHRASE),
	FUSE_OPT_KEY("-V",             KEY_VERSION),
	FUSE_OPT_KEY("--version",      KEY_VERSION),
	FUSE_OPT_KEY("-S ",            KEY_SYNCSECS),
	FUSE_OPT_KEY("--syncsecs ",    KEY_SYNCSECS),
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

    // NOTE - the value in 'arg' includes the option with any
    // intervening space removed, ie: "-Lutopfs.log".

	switch (key)
    {
    case KEY_BSID:
        utopfs.bsid = &arg[2];
        return 0;

    case KEY_LOGLEVEL:
        utopfs.loglevel = atoi(&arg[2]);
        return 0;

    case KEY_LOGPATH:
        utopfs.logpath = &arg[2];
        return 0;

    case KEY_MKFS:
        utopfs.size = atoll(&arg[2]);
        return 0;

    case KEY_FSID:
        utopfs.fsid = &arg[2];
        return 0;

    case KEY_PASSPHRASE:
        utopfs.passphrase = &arg[2];
        return 0;

    case KEY_SYNCSECS:
        utopfs.syncsecs = atof(&arg[2]);
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

	case KEY_FOREGROUND:
        utopfs.foreground = true;
        return 1;

	case KEY_HELP:
	case KEY_VERSION:
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
    // Setup defaults
    utopfs.argv0 = argv[0];
    utopfs.foreground = false;
    utopfs.lclconf = LCLCONF;
    utopfs.bsid = "FSBS";
    utopfs.logpath = "utopfs.log";
    utopfs.loglevel = -1;
    utopfs.size = 0;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, &utopfs, utopfs_opts, utopfs_opt_proc) == -1)
        fatal("option parsing failed");

    utopfs.mntpath = args.argv[args.argc-1];

    // When we are daemonized our path is changed to '/'.  If the
    // blockstore path is relative convert it to absolute ...
    //
    if (utopfs.path[0] != '/')
    {
        char cwdbuf[MAXPATHLEN];
        string cwd = getcwd(cwdbuf, sizeof(cwdbuf));
        utopfs.path = cwd + '/' + utopfs.path;
    }

    // Convert LCLCONF path to absolute.
    if (utopfs.lclconf[0] != '/')
    {
        char cwdbuf[MAXPATHLEN];
        string cwd = getcwd(cwdbuf, sizeof(cwdbuf));
        utopfs.lclconf = cwd + '/' + utopfs.lclconf;
    }

    // Convert the log path to absolute.
    if (utopfs.logpath[0] != '/')
    {
        char cwdbuf[MAXPATHLEN];
        string cwd = getcwd(cwdbuf, sizeof(cwdbuf));
        utopfs.logpath = cwd + '/' + utopfs.logpath;
    }

    // Convert the mount path to absolute.
    if (utopfs.mntpath[0] != '/')
    {
        char cwdbuf[MAXPATHLEN];
        string cwd = getcwd(cwdbuf, sizeof(cwdbuf));
        utopfs.mntpath = cwd + '/' + utopfs.mntpath;
    }

    utopfs_oper.init		= utopfs_init;
    utopfs_oper.destroy		= utopfs_destroy;
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
    utopfs_oper.statfs		= utopfs_statfs;
    utopfs_oper.readdir		= utopfs_readdir;
    utopfs_oper.access		= utopfs_access;
    utopfs_oper.utimens		= utopfs_utimens;

    return fuse_main(args.argc, args.argv, &utopfs_oper, NULL);
}
