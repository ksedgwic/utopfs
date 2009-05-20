#ifndef utp_Digest_h__
#define utp_Digest_h__

#include <iosfwd>

#include "Types.h"

namespace utp {

class Digest
{
public:
    Digest(void const * i_data, size_t i_size);

    uint8 const * data() const { return m_dig; }

    size_t size() const { return sizeof(m_dig); }

private:
    uint8		m_dig[32];
};

std::ostream & operator<<(std::ostream & strm, Digest const & i_dig);

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Digest_h__
