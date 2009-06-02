#include <ace/OS_NS_string.h>

#include "Base32.h"
#include "Digest.h"
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


BlockRef::BlockRef(Digest const & i_dig)
{
    // Use the digest for the first half ...
    ACE_OS::memcpy(m_ref, i_dig.data(), 16);

    // ... and random bytes for the second half.
    Random::fill(m_ref + 16, 16);
}

bool
BlockRef::operator!() const
{
    for (unsigned i = 0; i < sizeof(m_ref); ++i)
        if (m_ref[i])
            return false;
    return true;
}

bool
BlockRef::validate(uint8 const * i_data, size_t i_size) const
{
    // Compute the digest of the data block.
    Digest dig(i_data, i_size);

    // Does the first part match?
    return ACE_OS::memcmp(dig.data(), m_ref, 16) == 0;
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
