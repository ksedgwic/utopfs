#include "FSBlockStoreImpl.h"

namespace utp {

FSBlockStoreImpl::FSBlockStoreImpl()
{
}

FSBlockStoreImpl::~FSBlockStoreImpl()
{
}

size_t
FSBlockStoreImpl::bs_get_block(void * i_keydata,
                               size_t i_keysize,
                               void * o_outbuff,
                               size_t i_outsize)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
}

void
FSBlockStoreImpl::bs_put_block(void * i_keydata,
                               size_t i_keysize,
                               void * i_blkdata,
                               size_t i_blksize)
    throw(InternalError,
          ValueError)
{
}

void
FSBlockStoreImpl::bs_del_block(void * i_keydata,
                               size_t i_keysize)
    throw(InternalError,
          NotFoundError)
{
}

} // end namespace utp
