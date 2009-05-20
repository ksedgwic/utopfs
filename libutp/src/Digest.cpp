#include <iostream>

#include <openssl/sha.h>

#include "Base32.h"
#include "Digest.h"

using namespace std;
using namespace utp;

namespace utp {

Digest::Digest(void const * i_data, size_t i_size)
{
    SHA256((unsigned char const *) i_data, i_size, m_dig);
}

ostream & operator<<(ostream & ostrm, Digest const & i_dig)
{
    ostrm << Base32::encode(i_dig.data(), i_dig.size());
    return ostrm;
}


} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
