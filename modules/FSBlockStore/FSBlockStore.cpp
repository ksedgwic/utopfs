#include <sstream>
#include <string>

#include "Log.h"

#include "FSBlockStore.h"
#include "fsbslog.h"

using namespace std;
using namespace utp;

namespace FSBS {

FSBlockStore::FSBlockStore()
{
    LOG(lgr, 5, "CTOR");
}

FSBlockStore::~FSBlockStore()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

size_t
FSBlockStore::bs_get_block(void * i_keydata,
                           size_t i_keysize,
                           void * o_outbuff,
                           size_t i_outsize)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
    LOG(lgr, 5, "bs_get_block");

    throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_get_block unimplemented");
}

void
FSBlockStore::bs_put_block(void * i_keydata,
                           size_t i_keysize,
                           void * i_blkdata,
                           size_t i_blksize)
    throw(InternalError,
          ValueError)
{
    LOG(lgr, 5, "bs_put_block");

    throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_put_block unimplemented");
}

void
FSBlockStore::bs_del_block(void * i_keydata,
                           size_t i_keysize)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 5, "bs_del_block");

    throwstream(InternalError, FILELINE
                << "FSBlockStore::bs_del_block unimplemented");
}

} // namespace FSBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
