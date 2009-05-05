#include "FSAssocStoreImpl.h"

namespace utp {

FSAssocStoreImpl::FSAssocStoreImpl()
{
}

FSAssocStoreImpl::~FSAssocStoreImpl()
{
}

size_t
FSAssocStoreImpl::as_get_block(void * i_keydata,
                               size_t i_keysize,
                               void * o_outbuff,
                               size_t i_outsize)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
}

void
FSAssocStoreImpl::as_put_block(void * i_keydata,
                               size_t i_keysize,
                               void * i_blkdata,
                               size_t i_blksize)
    throw(InternalError,
          ValueError)
{
}

void
FSAssocStoreImpl::as_del_block(void * i_keydata,
                               size_t i_keysize)
    throw(InternalError,
          NotFoundError)
{
}

} // end namespace utp
