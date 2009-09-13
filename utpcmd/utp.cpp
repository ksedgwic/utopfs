#include <iostream>
#include <sstream>
#include <string>

#include <ace/Get_Opt.h>
#include <ace/LSOCK_Connector.h>
#include <ace/Service_Config.h>
#include <ace/UNIX_Addr.h>

#include <Types.h>
#include <BlockStoreFactory.h>
#include <FileSystemFactory.h>

using namespace std;
using namespace utp;

enum commands
{
    CMD_NONE,
    CMD_COMPACT,
    CMD_MKBS,
    CMD_RMBS,
    CMD_MKFS,
};

string			g_argv0;
string			g_cmdstr;
unsigned		g_cmd = CMD_NONE;
StringSeq		g_cmdargs;
string			g_bstype;
uint64			g_bssize;
string			g_fsid;
string			g_pass;

#define fatal(__msg)                            \
    do {                                        \
        cerr << "FATAL: " << __msg << endl;     \
        exit(1);                                \
    } while (false)

string
usage()
{
    ostringstream ostrm;

    ostrm << "Usage: " << g_argv0 << " [generic-options] [command]" << endl
          << endl
          << "  generic options:" << endl
          << "    --help                     display usage and exit" << endl
          << endl
          << "  commands:" << endl
          << "    mkbs                       Create Blockstore" << endl
          << "        --bstype=<bstype>      blockstore type" << endl
          << "        --bssize=<size>        blockstore size (bytes)" << endl
          << "        [<bsargs> ...]         blockstore dependent args" << endl
          << endl
          << "    rmbs                       Destroy Blockstore" << endl
          << "        --bstype=<bstype>      blockstore type" << endl
          << "        [<bsargs> ...]         blockstore dependent args" << endl
          << endl
          << "    mkfs                       Create Filesystem"  << endl
          << "        --fsid=<fsid>          filesytem id" << endl
          << "        --pass=<passphrase>    filesytem passphrase" << endl
          << "        --bstype=<bstype>      blockstore type" << endl
          << "        [--bssize=<size>]      blockstore size (creates)" << endl
          << "        [<bsargs> ...]         blockstore dependent args" << endl
          << endl
          << "    compact                    Compact Filesystem" << endl
        ;
    return ostrm.str();
}

void
parse_args(int & argc, char ** & argv)
{
    ACE_Get_Opt getopt(argc, argv, "-:");
    getopt.long_option("help", ACE_Get_Opt::NO_ARG);
    getopt.long_option("bstype", ACE_Get_Opt::ARG_REQUIRED);
    getopt.long_option("bssize", ACE_Get_Opt::ARG_REQUIRED);
    getopt.long_option("fsid", ACE_Get_Opt::ARG_REQUIRED);
    getopt.long_option("pass", ACE_Get_Opt::ARG_REQUIRED);

    bool cmdseen = false;
    char * endp;

    for (int c; (c = getopt()) != EOF; )
    {
        switch (c)
        {
        case 0:		// Long argument.
            {
                string lopt = getopt.long_option();
                if (lopt == "help")
                {
                    cerr << usage() << endl;
                    exit(0);
                }
                else if (lopt == "bssize")
                {
                    switch (g_cmd)
                    {
                    case CMD_NONE:
                        fatal("--bssize option invalid before command"
                              << endl << usage());

                    case CMD_MKBS:
                    case CMD_MKFS:
                        g_bssize = strtoull(getopt.opt_arg(), &endp, 0);
                        if (*endp != NULL)
                            fatal("invalid --bssize value \""
                                  << getopt.opt_arg() << "\"");
                        break;

                    default:
                        fatal("--bssize option invalid with " << g_cmdstr
                              << endl << usage());
                    }
                }
                else if (lopt == "bstype")
                {
                    switch (g_cmd)
                    {
                    case CMD_NONE:
                        fatal("--bstype option invalid before command"
                              << endl << usage());

                    case CMD_MKBS:
                    case CMD_RMBS:
                    case CMD_MKFS:
                        g_bstype = getopt.opt_arg();
                        break;

                    default:
                        fatal("--bstype option invalid with " << g_cmdstr
                              << endl << usage());
                    }
                }
                else if (lopt == "fsid")
                {
                    switch (g_cmd)
                    {
                    case CMD_NONE:
                        fatal("--fsid option invalid before command"
                              << endl << usage());

                    case CMD_MKFS:
                        g_fsid = getopt.opt_arg();
                        break;

                    default:
                        fatal("--fsid option invalid with " << g_cmdstr
                              << endl << usage());
                    }
                }
                else if (lopt == "pass")
                {
                    switch (g_cmd)
                    {
                    case CMD_NONE:
                        fatal("--pass option invalid before command"
                              << endl << usage());

                    case CMD_MKFS:
                        g_pass = getopt.opt_arg();
                        break;

                    default:
                        fatal("--pass option invalid with " << g_cmdstr
                              << endl << usage());
                    }
                }
                else
                {
                    fatal("Unknown option: \"" << lopt << "\""
                          << endl << usage());
                }
            }
            break;

        case '?':
            fatal("unrecognized option: \"" << argv[getopt.opt_ind()-1] << '"'
                  << endl << usage());

        case ':':	// Missing required argument.
            fatal("missing argument to \"" << getopt.last_option() << "\""
                  << endl << usage());

        case 1:		// Non-option.
            {
                string arg = getopt.opt_arg();

                if (!cmdseen)
                {
                    if (arg == "compact")
                        g_cmd = CMD_COMPACT;
                    else if (arg == "mkbs")
                        g_cmd = CMD_MKBS;
                    else if (arg == "rmbs")
                        g_cmd = CMD_RMBS;
                    else if (arg == "mkfs")
                        g_cmd = CMD_MKFS;
                    else
                        fatal("unrecognized command: \"" << arg << "\""
                              << endl << usage());

                    g_cmdstr = arg;
                    cmdseen = true;
                }
                else
                {
                    switch (g_cmd)
                    {
                    case CMD_MKBS:
                    case CMD_RMBS:
                    case CMD_MKFS:
                        g_cmdargs.push_back(arg);
                        break;

                    default:
                        fatal("unrecognized argument: \"" << arg << "\""
                              << endl << usage());
                    }
                }
            }
            break;
        }
    }
}

void
do_compact()
{
    // Find the control socket.
    char buf[MAXPATHLEN];
    if (!getcwd(buf, sizeof(buf)))
        fatal("getcwd failed: " << ACE_OS::strerror(errno));

    string path(buf);
    string ctrlpath;

    while (true)
    {
        // Is the control socket here?
        ctrlpath = path + "/.utopfs/control";
        ACE_stat sb;
        int rv = ACE_OS::stat(ctrlpath.c_str(), &sb);
        if (rv == 0)
        {
            // Needs to be a socket.
            if (!S_ISSOCK(sb.st_mode))
                fatal(ctrlpath << " is not socket");
            break;
        }

        // Shorten the path by one directory.
        string::size_type pos = path.find_last_of('/');
        if (pos == string::npos)
            fatal("control file not found; "
                  "are we in a mounted utopfs filesystem?");
        path = path.substr(0, pos);
    }

    ACE_LSOCK_Stream cli_stream;
    ACE_LSOCK_Connector con;
    ACE_UNIX_Addr remote_addr(ctrlpath.c_str());

    // Establish the connection with server.
    if (con.connect (cli_stream, remote_addr) == -1)
        fatal("connect failed: " << ACE_OS::strerror(errno));

    string cmdbuf("compact");
    if (cli_stream.send_n(cmdbuf.data(), cmdbuf.size()) == -1)
        fatal("send_n failed: " << ACE_OS::strerror(errno));

    // Explicitly close the writer-side of the connection.
    if (cli_stream.close_writer() == -1)
        fatal("close_writer failed: " << ACE_OS::strerror(errno));

    char retbuf[8192];
    int rv = cli_stream.recv(retbuf, sizeof(retbuf));
    if (rv == -1)
        fatal("recv_n failed: " <<  ACE_OS::strerror(errno));
    string retstr(retbuf, rv);

    // Close the connection completely.
    if (cli_stream.close () == -1)
        fatal("close failed: " <<  ACE_OS::strerror(errno));

    cout << retstr << endl;
}

void
do_mkbs()
{
    if (g_bstype.empty())
        fatal("missing --bstype argument with mkbs"
              << endl << usage());

    if (g_bssize == 0)
        fatal("missing --bssize argument with mkbs"
              << endl << usage());

    BlockStoreFactory::create(g_bstype, "__bstmp", g_bssize, g_cmdargs);
}

void
do_rmbs()
{
    if (g_bstype.empty())
        fatal("missing --bstype argument with rmbs"
              << endl << usage());

    BlockStoreFactory::destroy(g_bstype, g_cmdargs);
}

void
do_mkfs()
{
    if (g_bstype.empty())
        fatal("missing --bstype argument with mkfs"
              << endl << usage());

    if (g_fsid.empty())
        fatal("missing --fsid argument with mkfs"
              << endl << usage());

    if (g_pass.empty())
        fatal("missing --pass argument with mkfs"
              << endl << usage());

    BlockStoreHandle bsh;
    if (g_bssize == 0)
        bsh = BlockStoreFactory::open(g_bstype,
                                      "__bstmp",
                                      g_cmdargs);
    else
        bsh = BlockStoreFactory::create(g_bstype,
                                        "__bstmp",
                                        g_bssize,
                                        g_cmdargs);

    string uname = FileSystemFactory::mapuid(geteuid());
    string gname = FileSystemFactory::mapgid(getegid());

    // NOTE = We've hard-wired the UTFS module here.
    // NOTE - We've hard-wired an empty fs arg list here.
    //
    StringSeq fsargs;
    FileSystemFactory::mkfs("UTFS", bsh, g_fsid, g_pass, uname, gname, fsargs);
}

#define LCLCONF "./utp.conf"
#define SYSCONF "/etc/sysconfig/utopfs/utp.conf"
static void
init_modules()
{
    ACE_stat statbuf;

    // Build a synthetic command line.
    vector<string> args;
    args.push_back(g_argv0);

	// Enable ACE debugging
    // args.push_back("-d");

    // Is there a UTOPFS_SVCCONF env variable?
    char * svcconfenv = ACE_OS::getenv("UTP_SVCCONF");
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
    g_argv0 = argv[0];

    parse_args(argc, argv);

    init_modules();

    switch (g_cmd)
    {
    case CMD_COMPACT:
        do_compact();
        break;

    case CMD_MKBS:
        do_mkbs();
        break;

    case CMD_RMBS:
        do_rmbs();
        break;

    case CMD_MKFS:
        do_mkfs();
        break;

    case CMD_NONE:
        fatal("no command specified" << endl << usage());
    }

    return 0;
}

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
