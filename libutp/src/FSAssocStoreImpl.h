#ifndef utp_FSBlockStoreImpl_h__
#define utp_FSBlockStoreImpl_h__

/// @file FSBlockStoreImpl.h
/// FileSystem Based BlockStore Implementation.

#include "BlockStore.h"

namespace utp {

class UTP_EXP FSBlockStoreImpl : public BlockStore
{
public:
    FSBlockStoreImpl();

    virtual ~FSBlockStoreImpl();

    virtual size_t bs_get_block(void * i_keydata,
                                size_t i_keysize,
                                void * o_outbuff,
                                size_t i_outsize)
        throw(InternalError,
              NotFoundError,
              ValueError);

    virtual void bs_put_block(void * i_keydata,
                              size_t i_keysize,
                              void * i_blkdata,
                              size_t i_blksize)
        throw(InternalError,
              ValueError);

    virtual void bs_del_block(void * i_keydata,
                              size_t i_keysize)
        throw(InternalError,
              NotFoundError);
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_FSBlockStoreImpl_h__
