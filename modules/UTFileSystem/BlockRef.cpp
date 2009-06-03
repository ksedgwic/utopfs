#include <ace/OS_NS_string.h>

#include "Base32.h"
#include "Digest.h"
#include "Except.h"
#include "Log.h"
#include "Random.h"

#include "utfslog.h"

#include "BlockRef.h"

using namespace std;
using namespace utp;

namespace UTFS {

BlockRef::BlockRef()
{
    ACE_OS::memset(m_ref, '\0', sizeof(m_ref));
}


BlockRef::BlockRef(Digest const & i_dig, uint8 const * i_ivp)
{
    // Use the digest for the first half.
    ACE_OS::memcpy(m_ref, i_dig.data(), 16);

    // Use the initvec for the second half.
    ACE_OS::memcpy(m_ref + 16, i_ivp, 16);
}

BlockRef::BlockRef(string const & i_blkrefstr)
{
    if (i_blkrefstr.size() != 32)
        throwstream(InternalError, FILELINE
                    << "stored blkref is wrong size: " << i_blkrefstr.size());

    ACE_OS::memcpy(m_ref, i_blkrefstr.data(), sizeof(m_ref));
}

bool
BlockRef::operator!() const
{
    for (unsigned i = 0; i < sizeof(m_ref); ++i)
        if (m_ref[i])
            return false;
    return true;
}

void
BlockRef::validate(uint8 const * i_data, size_t i_size) const
    throw(VerificationError)
{
    // Compute the digest of the data block.
    Digest dig(i_data, i_size);

    // Does the first part match?
    if (ACE_OS::memcmp(dig.data(), m_ref, 16) != 0)
        throwstream(VerificationError, *this);
}

ostream & operator<<(ostream & ostrm, BlockRef const & i_ref)
{
    // Since this is a human friendly version just show first N bytes ...
    ostrm << Base32::encode(i_ref.data(), i_ref.size()).substr(0, 8) << "...";
    return ostrm;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
