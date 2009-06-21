#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <ace/Reactor.h>

#include "fuselog.h"
#include "Controller.h"

using namespace std;
using namespace utp;

Controller::Controller(string const & i_sockpath)
    : m_sockpath(i_sockpath)
    , m_reactor(ACE_Reactor::instance())
{
    LOG(lgr, 4, "CTOR");
}

Controller::~Controller()
{
    LOG(lgr, 4, "DTOR");
}

void
Controller::init()
{
    LOG(lgr, 4, "init");

    // Schedule a timeout to get the socket open on a different thread.
    //
    // BOGUS - we need a couple of seconds delay here or we deadlock.
    // Surely there is a better way?  Can we trap some other FUSE
    // event and init then?
    //
    m_reactor->schedule_timer(this, NULL, ACE_Time_Value(2, 0));
}

int
Controller::handle_timeout(ACE_Time_Value const & current_time,
                           void const * act)
{
    LOG(lgr, 4, "handle_timeout");

    int sock, msgsock, rval;
    struct sockaddr_un server;
    char buf[1024];

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("opening stream socket");
        exit(1);
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, m_sockpath.c_str());
    if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)))
    {
        perror("binding stream socket");
        exit(1);
    }
    printf("Socket has name %s\n", server.sun_path);

#if 0
    listen(sock, 5);
    for (;;)
    {
        msgsock = accept(sock, 0, 0);
        if (msgsock == -1)
            perror("accept");
        else do
             {
                 bzero(buf, sizeof(buf));
                 if ((rval = read(msgsock, buf, 1024)) < 0)
                     perror("reading stream message");
                 else if (rval == 0)
                     printf("Ending connection\n");
                 else 
                     printf("-->%s\n", buf);
             } while (rval > 0);
        close(msgsock);
    }
    close(sock);
    unlink(NAME);
    return 0;
#endif

    return 0;
}

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
