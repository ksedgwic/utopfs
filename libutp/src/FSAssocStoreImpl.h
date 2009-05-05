#ifndef utp_FSAssocStoreImpl_h__
#define utp_FSAssocStoreImpl_h__

/// @file FSAssocStoreImpl.h
/// FileSystem Based AssocStore Implementation.

#include "AssocStore.h"

namespace utp {

class UTP_EXP FSAssocStoreImpl : public AssocStore
{
public:
    FSAssocStoreImpl();

    virtual ~FSAssocStoreImpl();

    virtual size_t as_get_block(void * i_keydata,
                                size_t i_keysize,
                                void * o_outbuff,
                                size_t i_outsize)
        throw(InternalError,
              NotFoundError,
              ValueError);

    virtual void as_put_block(void * i_keydata,
                              size_t i_keysize,
                              void * i_blkdata,
                              size_t i_blksize)
        throw(InternalError,
              ValueError);

    virtual void as_del_block(void * i_keydata,
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

#endif // utp_FSAssocStoreImpl_h__
