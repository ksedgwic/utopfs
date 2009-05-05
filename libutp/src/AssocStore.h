#ifndef utp_AssocStore_h__
#define utp_AssocStore_h__

/// @file AssocStore.h
/// Abstract AssocStore Interface.

#include <string>

#include "utpexp.h"
#include "utpfwd.h"

#include "Except.h"
#include "RC.h"

namespace utp {

class UTP_EXP AssocStore : public virtual RCObj
{
public:
    /// Assign the singleton instance
    ///
    static void instance(AssocStoreHandle const & i_ash);

    /// Retrieve the singleton instance
    ///
    static AssocStoreHandle instance();

    /// Destructor
    ///
    virtual ~AssocStore();

    /// Get a block.
    ///
    /// @param[in] i_keydata Pointer to the key data.
    /// @param[in] i_keysize Size of the key data.
    /// @param[out] o_outbuff Pointer to buffer to store the data.
    /// @param[in] i_outsize Size of the output buffer.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified key was not found.
    /// @throw ValueError One of the arguments is out of range.
    ///
    /// @return The size of the returned block.
    ///
    virtual size_t as_get_block(void * i_keydata,
                                size_t i_keysize,
                                void * o_outbuff,
                                size_t i_outsize)
        throw(InternalError,
              NotFoundError,
              ValueError) = 0;

    /// Put a block, replaces existing.
    ///
    /// @param[in] i_keydata Pointer to the key data.
    /// @param[in] i_keysize Size of the key data.
    /// @param[in] i_blkdata Pointer to the block data.
    /// @param[in] i_blksize Size of the block data.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw ValueError One of the arguments is out of range.
    ///
    virtual void as_put_block(void * i_keydata,
                              size_t i_keysize,
                              void * i_blkdata,
                              size_t i_blksize)
        throw(InternalError,
              ValueError) = 0;

    /// Delete a block.
    ///
    /// @param[in] i_keydata Pointer to the key data.
    /// @param[in] i_keysize Size of the key data.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified key was not found.
    ///
    virtual void as_del_block(void * i_keydata,
                              size_t i_keysize)
        throw(InternalError,
              NotFoundError) = 0;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_AssocStore_h__
