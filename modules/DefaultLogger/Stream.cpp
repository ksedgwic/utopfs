#include <iostream>
#include <sstream>

#include <ace/Synch.h>

#include "Log.h"

#include "Stream.h"
#include "Formatter.h"

using namespace std;
using namespace utp;

namespace DFLG {

Stream::Stream(int i_level,
               ostream & i_ostrm,
               bool i_flushflag)
    : LoggerBase(i_level, Formatter::getInstance())
    , m_ostrm(&i_ostrm)
    , m_isowned(false)
    , m_flushflag(i_flushflag)
{
}

Stream::Stream(int i_level,
               ostream * i_ostrm,
               bool i_flushflag)
    : LoggerBase(i_level, Formatter::getInstance())
    , m_ostrm(i_ostrm)
    , m_isowned(true)
    , m_flushflag(i_flushflag)
{
}

Stream::~Stream()
{
    if (m_isowned)
        delete m_ostrm;
}

void
Stream::debug_str(int i_level,
                  const LogFieldSeq & i_valseq)
{
    if (m_level < i_level)
        return;

    ostringstream buffer;
    buffer << formatter()->format(i_valseq) << endl;
        
    // lock it out to avoid printing at same time to log
    ACE_Guard<ACE_Thread_Mutex> guard(m_sl_mutex);
    *m_ostrm << buffer.str();
    if (m_flushflag)
        m_ostrm->flush();
}

} // end namespace DFLG

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
