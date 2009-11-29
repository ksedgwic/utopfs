#ifndef UTFS_UTStats_h__
#define UTFS_UTStats_h__

/// @file UTStats.h
/// Utopia FileSystem Stats Object.
///
/// Convenience bundle of stats needed for working in the UTFS.

#include "utpfwd.h"

#include "BlockStore.h"
#include "BlockCipher.h"

#include "utfsexp.h"
#include "utfsfwd.h"

namespace UTFS {

struct UTFS_EXP UTStats
{
    // Reads and Writes (upward interface)
    utp::AtomicLong				m_nrdops;
    utp::AtomicLong				m_nwrops;
    utp::AtomicLong				m_nrdbytes;
    utp::AtomicLong				m_nwrbytes;

    // Gets and Puts (downward interface)
    utp::AtomicLong				m_ngops;
    utp::AtomicLong 			m_npops;
    utp::AtomicLong 			m_ngbytes;
    utp::AtomicLong 			m_npbytes;

    // Constructor clears.
    UTStats()
        : m_nrdops(0)
        , m_nwrops(0)
        , m_nrdbytes(0)
        , m_nwrbytes(0)
        , m_ngops(0)
        , m_npops(0)
        , m_ngbytes(0)
        , m_npbytes(0)
    {
    }

};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_UTStats_h__
