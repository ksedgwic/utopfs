#ifndef utp_Random_h__
#define utp_Random_h__

#include <iosfwd>

#include "utpexp.h"
#include "utpfwd.h"

#include "Types.h"

namespace utp {

class UTP_EXP Random
{
public:
    /// Fill memory with random bytes.
    ///
    /// @param[out] o_ptr Pointer to the memory to be filled.
    /// @param[in] i_size Number of bytes to be filled.
    ///
    static void fill(void * o_ptr, size_t i_size);
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Random_h__
