#ifndef utp_Base64_h__
#define utp_Base64_h__

#include <string>

#include "utpexp.h"
#include "utpfwd.h"

#include "Types.h"

namespace utp {
#include <string>

class UTP_EXP Base64
{
public:
    static std::string encode(void const * i_ptr, size_t i_len);

    static std::string decode(std::string const & enc);
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Base64_h__
