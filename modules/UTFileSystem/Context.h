#ifndef UTFS_Context_h__
#define UTFS_Context_h__

/// @file Context.h
/// Utopia FileSystem Context Object.
///
/// Convenience bundle of state needed for working in the UTFS.

#include "utpfwd.h"

#include <ace/Thread_Mutex.h>
#include <ace/Condition_Thread_Mutex.h>

#include "BlockStore.h"
#include "BlockCipher.h"

#include "utfsexp.h"
#include "utfsfwd.h"

namespace UTFS {

class UTFS_EXP Context
{
 public:
    Context();

    UTFileSystem *				m_utfsp;
    utp::BlockStoreHandle		m_bsh;
    utp::BlockCipher			m_cipher;
    DataBlockNodeHandle			m_zdatobj;
    IndirectBlockNodeHandle		m_zsinobj;
    DoubleIndBlockNodeHandle	m_zdinobj;
    TripleIndBlockNodeHandle	m_ztinobj;

    // This mutex/conditionvar is used to protect folks waiting on the
    // m_putsout member value.
    //
    mutable ACE_Thread_Mutex	m_ctxtmutex;
    ACE_Condition_Thread_Mutex	m_ctxtcond;
    bool						m_waiters;

    size_t						m_putsout;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_Context_h__
