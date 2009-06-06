#ifndef UTFS_Context_h__
#define UTFS_Context_h__

/// @file Context.h
/// Utopia FileSystem Context Object.
///
/// Convenience bundle of state needed for working in the UTFS.

#include "utpfwd.h"

#include "BlockStore.h"
#include "BlockCipher.h"

#include "utfsexp.h"
#include "utfsfwd.h"

namespace UTFS {

struct UTFS_EXP Context
{
    utp::BlockStoreHandle		m_bsh;
    utp::BlockCipher			m_cipher;
    DataBlockNodeHandle			m_zdatobj;
    IndirectBlockNodeHandle		m_zsinobj;
    DoubleIndBlockNodeHandle	m_zdinobj;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_Context_h__
