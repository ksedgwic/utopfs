#include <iostream>
#include <sstream>
#include <string>

#include <ace/Get_Opt.h>
#include <ace/LSOCK_Connector.h>
#include <ace/UNIX_Addr.h>

using namespace std;

enum commands
{
    CMD_NONE,
    CMD_COMPACT
};

string g_argv0;
unsigned g_cmd = CMD_NONE;

#define fatal(__msg)                            \
    do {                                        \
        cerr << __msg << endl;                  \
        exit(1);                                \
    } while (false)

string
usage()
{
    ostringstream ostrm;

    ostrm << "Usage: " << g_argv0 << " [options] [command]" << endl
          << "  options:" << endl
          << "    --help          display usage and exit" << endl
          << "  command:" << endl
          << "    compact         compact filesystem" << endl
        ;
    return ostrm.str();
}

void
parse_args(int & argc, char ** & argv)
{
    ACE_Get_Opt getopt(argc, argv, "-:");
    getopt.long_option("help", ACE_Get_Opt::NO_ARG);

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
                string cmd = getopt.opt_arg();

                if (cmd == "compact")
                    g_cmd = CMD_COMPACT;
                else
                    fatal("unrecognized command: \"" << cmd << "\""
                          << endl << usage());
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

int
main(int argc, char ** argv)
{
    g_argv0 = argv[0];

    parse_args(argc, argv);

    switch (g_cmd)
    {
    case CMD_COMPACT:
        do_compact();
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
