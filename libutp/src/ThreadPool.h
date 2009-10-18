#ifndef utp_ThreadPool_h__
#define utp_ThreadPool_h__

/// @file ThreadPool.h
/// Utopia FileSystem ThreadPool Object.
///
/// ThreadPool Object for Utopfs FileSystem.

#include <string>

#include <ace/Reactor.h>
#include <ace/Task.h>

#include "Except.h"

#include "utpfwd.h"
#include "utpexp.h"

namespace utp {

class UTP_EXP ThreadPool : public ACE_Task_Base
{
public:
    ThreadPool(ACE_Reactor * i_reactor,
               std::string const & i_tpname);
    
    ~ThreadPool();

    virtual int svc(void);

    void init(int i_numthreads);

    void term();

private:
    ACE_Reactor *		m_reactor;
    bool				m_running;
    std::string			m_tpname;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_ThreadPool_h__
