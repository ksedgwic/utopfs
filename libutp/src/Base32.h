#ifndef utp_Base32_h__
#define utp_Base32_h__

#include <string>

#include "Types.h"

namespace utp {

class Base32
{
public:
    static std::string const encode(void const * i_data, size_t i_size);

    static void encode(uint8 const * i_data,
                       size_t const & i_size,
                       std::string & o_encoded);

    static std::string const encode(OctetSeq const & i_data);
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Base32_h__
