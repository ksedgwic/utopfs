#include <openssl/rand.h>

#include "Except.h"
#include "Random.h"

using namespace std;
using namespace utp;

namespace utp {

void
Random::fill(void * o_ptr, size_t i_size)
{
    int rv = RAND_pseudo_bytes((unsigned char *) o_ptr, i_size);
    if (rv != 1)
        throwstream(InternalError, FILELINE
                    << "trouble in RAND_pseudo_bytes");
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
