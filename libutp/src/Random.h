#ifndef utp_Random_h__
#define utp_Random_h__

#include <iosfwd>

#include "Types.h"

namespace utp {

class Random
{
public:
    static void bytes(void * o_ptr, size_t i_size);
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Random_h__
