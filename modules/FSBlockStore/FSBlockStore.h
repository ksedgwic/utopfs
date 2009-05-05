#ifndef FSBlockStore_h__
#define FSBlockStore_h__

/// @file FSBlockStore.h
/// FileSystem BlockStore Instance.

#include <string>

#include "utpfwd.h"

#include "BlockStore.h"

#include "fsbsexp.h"

namespace FSBS {

class FSBS_EXP FSBlockStore : public utp::BlockStore
{
public:
    FSBlockStore();

    virtual ~FSBlockStore();

    // BlockStore methods.

    virtual size_t bs_get_block(void * i_keydata,
                                size_t i_keysize,
                                void * o_outbuff,
                                size_t i_outsize)
        throw(utp::InternalError,
              utp::NotFoundError,
              utp::ValueError);

    virtual void bs_put_block(void * i_keydata,
                              size_t i_keysize,
                              void * i_blkdata,
                              size_t i_blksize)
        throw(utp::InternalError,
              utp::ValueError);

    virtual void bs_del_block(void * i_keydata,
                              size_t i_keysize)
        throw(utp::InternalError,
              utp::NotFoundError);

private:
};

} // namespace FSBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // FSBlockStore_h__
