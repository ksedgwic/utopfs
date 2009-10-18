#include <stdexcept>

#include "ThreadPool.h"

#include "utplog.h"

using namespace std;

namespace utp {

    ThreadPool::ThreadPool(ACE_Reactor * i_reactor, string const & i_tpname)
    : m_reactor(i_reactor)
    , m_tpname(i_tpname)
{
    LOG(lgr, 4, "ThreadPool CTOR " << m_tpname);
}
    
ThreadPool::~ThreadPool()
{
    LOG(lgr, 4, "ThreadPool DTOR " << m_tpname);
}

int
ThreadPool::svc(void)
{
    LOG(lgr, 4, "ThreadPool " << m_tpname << " thread starting");

    // Run the Reactor event loop.  Catch exceptions and report but keep
    // the threads running ...
    //
    while (m_running)
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

    LOG(lgr, 4, "ThreadPool " << m_tpname << " thread finished");
    return 0;
}

void
ThreadPool::init(int i_numthreads)
{
    LOG(lgr, 4, "ThreadPool " << m_tpname << " init " << i_numthreads);
    m_running = true;
    
    if (activate(THR_NEW_LWP | THR_JOINABLE, i_numthreads) != 0)
        throwstream(InternalError, FILELINE
                    << "trouble activating ThreadPool");
}

void
ThreadPool::term()
{
    LOG(lgr, 4, "ThreadPool " << m_tpname << " term starting");

    m_running = false;
    m_reactor->end_reactor_event_loop();
    wait();

    LOG(lgr, 4, "ThreadPool " << m_tpname << " term finished");
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
