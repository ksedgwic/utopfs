#ifndef DFLG_Stream_h__
#define DFLG_Stream_h__

/// @file Stream.h
/// Defines Stream based Logger Implementation

#include <iostream>

#include <ace/Synch.h>

#include "Log.h"
#include "LoggerBase.h"

#include "dflgexp.h"

namespace DFLG {

/// Stream-based Logger Implementation.
class DFLG_EXP Stream : public utp::LoggerBase
{
public:
    /// Constructor which doesn't own the stream.
    Stream(int i_level, std::ostream & i_ostrm, bool i_flushflag);

    /// Constructor which assumes ownership of the stream.
    Stream(int i_level, std::ostream * i_ostrm, bool i_flushflag);

    /// Destructor.
    virtual ~Stream();

    virtual void debug_str(int i_level,
                           utp::LogFieldSeq const & i_valseq);

private:
    mutable ACE_Thread_Mutex m_sl_mutex;
    std::ostream *	         m_ostrm;
    bool			         m_isowned;
    bool			         m_flushflag;
};
    
} // end namespace DFLG

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // DFLG_Stream_h__
