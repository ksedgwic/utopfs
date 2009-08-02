#ifndef utp_MD5_h__
#define utp_MD5_h__

#include <iosfwd>
#include <string>

#include "Types.h"

namespace utp {

class MD5
{
public:
    /// Compute digest from range of memory.
    MD5(void const * i_data, size_t i_size);

    /// Cast digest to base64 string.
    operator char const * () const { return m_digstr.c_str(); }

private:
    std::string		m_digstr;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_MD5_h__
