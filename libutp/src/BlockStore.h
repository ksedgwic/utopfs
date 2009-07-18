#ifndef utp_BlockStore_h__
#define utp_BlockStore_h__

/// @file BlockStore.h
/// Abstract BlockStore Interface.

#include <string>
#include <vector>

#include "utpexp.h"
#include "utpfwd.h"

#include "Except.h"
#include "RC.h"
#include "Types.h"

namespace utp {

class UTP_EXP BlockStore : public virtual RCObj
{
public:
    /// List of keys.
    typedef std::vector<OctetSeq> KeySeq;

    /// Destructor
    ///
    virtual ~BlockStore();

    /// Create a block store.
    ///
    /// @param[in] i_size Size of the block store data in bytes.
    /// @param[in] i_path Path to the block store.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw ValueError One of the arguments is out of range.
    ///
    virtual void bs_create(size_t i_size, std::string const & i_path)
        throw(NotUniqueError,
              InternalError,
              ValueError) = 0;

    /// Open a block store.
    ///
    /// @param[in] i_path Path to the block store.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified key was not found.
    ///
    virtual void bs_open(std::string const & i_path)
        throw(InternalError,
              NotFoundError) = 0;

    /// Close a block store.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_close()
        throw(InternalError) = 0;

    struct Stat
    {
        off_t	bss_size;	//< Total data size in bytes.
        off_t	bss_free;	//< Uncommitted size in bytes.
    };

    /// Return block store statistics.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_stat(Stat & o_stat)
        throw(InternalError) = 0;


    /// Get a block via blocking interface.
    ///
    /// @note This interface is a wrapper on the nonblocking
    ///       interface.
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
    size_t bs_get_block(void const * i_keydata,
                        size_t i_keysize,
                        void * o_outbuff,
                        size_t i_outsize)
        throw(InternalError,
              NotFoundError,
              ValueError);

    /// Completion function interface for non-blocking get.
    ///
    class BlockGetCompletion
    {
    public:
        virtual void bg_complete(void const * i_keydata,
                                 size_t i_keysize,
                                 size_t i_blksize) = 0;

        virtual void bg_error(void const * i_keydata,
                              size_t i_keysize,
                              Exception const & i_exp) = 0;
    };

    /// Get block via non-blocking interface.
    ///
    virtual void bs_get_block_async(void const * i_keydata,
                                    size_t i_keysize,
                                    void * o_buffdata,
                                    size_t i_buffsize,
                                    BlockGetCompletion & i_cmpl)
        throw(InternalError,
              ValueError) = 0;

    /// Put a block via blocking interface.
    ///
    /// The inserted block replaces existing.
    ///
    /// @param[in] i_keydata Pointer to the key data.
    /// @param[in] i_keysize Size of the key data.
    /// @param[in] i_blkdata Pointer to the block data.
    /// @param[in] i_blksize Size of the block data.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw ValueError One of the arguments is out of range.
    /// @throw NoSpaceError Out of space to store block.
    ///
    void bs_put_block(void const * i_keydata,
                      size_t i_keysize,
                      void const * i_blkdata,
                      size_t i_blksize)
        throw(InternalError,
              ValueError,
              NoSpaceError);

    /// Completion function interface for non-blocking put.
    ///
    class BlockPutCompletion
    {
    public:
        virtual void bp_complete(void const * i_keydata,
                                 size_t i_keysize) = 0;

        virtual void bp_error(void const * i_keydata,
                              size_t i_keysize,
                              Exception const & i_exp) = 0;
    };

    /// Put block via non-blocking interface.
    ///
    virtual void bs_put_block_async(void const * i_keydata,
                                    size_t i_keysize,
                                    void const * i_blkdata,
                                    size_t i_blksize,
                                    BlockPutCompletion & i_cmpl)
        throw(InternalError,
              ValueError) = 0;

    /// Start a refresh cycle.
    ///
    /// @param[in] i_rid A unique refresh cycle identifier.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotUniqueError The specified refresh id is already in use.
    ///
    virtual void bs_refresh_start(utp::uint64 i_rid)
        throw(InternalError,
              NotUniqueError) = 0;

    /// Refresh a list of blocks, return list of any missing.
    ///
    /// Refreshing a block updates it's access timestamp, moving it to
    /// the front of the LRU queue.
    ///
    /// @param[in] i_rid The unique refresh cycle id.
    /// @param[in] i_keys List of keys for blocks to refresh.
    /// @param[out] o_missing Keys which were not present.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified refresh id was not found.
    ///
    virtual void bs_refresh_blocks(utp::uint64 i_rid,
                                   KeySeq const & i_keys,
                                   KeySeq & o_missing)
        throw(InternalError,
              NotFoundError) = 0;
        
    /// Finish a refresh cycle.
    ///
    /// @param[in] i_rid The unique refresh cycle identifier.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified refresh id was not found.
    ///
    virtual void bs_refresh_finish(utp::uint64 i_rid)
        throw(InternalError,
              NotFoundError) = 0;

    /// Ensures blocks are persisted.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_sync()
        throw(InternalError) = 0;
        
        
    
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_BlockStore_h__
