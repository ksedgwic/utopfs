#ifndef utp_BlockStore_h__
#define utp_BlockStore_h__

/// @file BlockStore.h
/// Abstract BlockStore Interface.

#include <string>
#include <vector>

#include "utpexp.h"
#include "utpfwd.h"

#include "HeadNode.pb.h"
#include "Except.h"
#include "RC.h"
#include "Types.h"

namespace utp {

class UTP_EXP BlockStore : public virtual RCObj
{
public:
    /// Sequence of keys.
    typedef std::vector<OctetSeq> KeySeq;

    /// Sequence of Signed Head Nodes
    typedef std::deque<SignedHeadNode> SignedHeadNodeSeq;

    /// Destructor
    ///
    virtual ~BlockStore();

    /// Create a block store.
    ///
    /// @param[in] i_size Size of the block store data in bytes.
    /// @param[in] i_args BlockStore specific arguments.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotUniqueError The blockstore already exists.
    /// @throw ValueError One of the arguments is out of range.
    ///
    virtual void bs_create(size_t i_size, StringSeq const & i_args)
        throw(InternalError,
              NotUniqueError,
              ValueError) = 0;

    /// Open a block store.
    ///
    /// @param[in] i_args BlockStore specific arguments.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified blockstore was not found.
    ///
    virtual void bs_open(StringSeq const & i_args)
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
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
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

    /// Put a block via blocking interface.
    ///
    /// The inserted block replaces existing.
    ///
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
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
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_rid The unique refresh cycle id.
    /// @param[in] i_keys List of keys for blocks to refresh.
    /// @param[out] o_missing Keys which were not present.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified refresh id was not found.
    ///
    void bs_refresh_blocks(utp::uint64 i_rid,
                           KeySeq const & i_keys,
                           KeySeq & o_missing)
        throw(InternalError,
              NotFoundError);
        
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

    /// Insert a SignedHeadNode into the BlockStore.
    ///
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_shn The SignedHeadNode
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    void bs_head_insert(SignedHeadNode const & i_shn)
        throw(InternalError);

    /// Return all nodes which follow a node.
    ///
    /// If the rootref field of the seed node is empty all nodes will
    /// be traversed.
    /// 
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_shn The SignedHeadNode to start at.
    /// @param[out] o_nodes Collection to hold returned nodes.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError No nodes were found.
    ///
    void bs_head_follow(SignedHeadNode const & i_seed,
                        SignedHeadNodeSeq & o_nodes)
        throw(InternalError,
              NotFoundError);

    /// Return only the extreme heads which follow a node.
    ///
    /// If the rootref field of the seed node is empty all extreme
    /// heads will be traversed.
    /// 
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_shn The SignedHeadNode to start at.
    /// @param[out] o_nodes Collection to hold returned nodes.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError No nodes were found.
    ///
    void bs_head_furthest(SignedHeadNode const & i_seed,
                          SignedHeadNodeSeq & o_nodes)
        throw(InternalError,
              NotFoundError);

    // ----------------------------------------------------------------
    // Asynchronous Interface Methods
    // ----------------------------------------------------------------

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

    /// Completion function interface for non-blocking refresh.
    ///
    class BlockRefreshCompletion
    {
    public:
        virtual void br_complete(void const * i_keydata,
                                 size_t i_keysize) = 0;

        virtual void br_missing(void const * i_keydata,
                                size_t i_keysize) = 0;
    };

    /// Refresh a block asynchronously, callback when done.
    ///
    /// @param[in] i_rid The unique refresh cycle id.
    /// @param[in] i_keydata Pointer to the key data.
    /// @param[in] i_keysize Size of the key data.
    /// @param[in] i_cmpl Completion function.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified refresh id was not found.
    ///
    virtual void bs_refresh_block_async(utp::uint64 i_rid,
                                        void const * i_keydata,
                                        size_t i_keysize,
                                        BlockRefreshCompletion & i_cmpl)
        throw(InternalError,
              NotFoundError) = 0;
        
    /// Completion callback interface for async SignedHeadNode
    /// insertion.
    ///
    class SignedHeadInsertCompletion
    {
    public:
        virtual void shi_complete(SignedHeadNode const & i_shn) = 0;

        virtual void shi_error(SignedHeadNode const & i_shn,
                               Exception const & i_exp) = 0;
    };

    /// Insert a SignedHeadNode into the BlockStore.
    ///
    /// @param[in] i_shn The SignedHeadNode to be inserted.
    /// @param[in] i_cmpl Completion function called when done.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_head_insert_async(SignedHeadNode const & i_shn,
                                      SignedHeadInsertCompletion & i_cmpl)
        throw(InternalError) = 0;

    /// Callback interface for SignedHeadNode follow and furthest
    /// traversals.
    ///
    class SignedHeadTraverseFunc
    {
    public:
        // Called on each node in the traversal.
        virtual void sht_node(SignedHeadNode const & i_shn) = 0;

        // Called when the traversal is complete.
        virtual void sht_complete() = 0;

        // Called instead if there is an error.
        virtual void sht_error(Exception const & i_exp) = 0;
    };

    /// Traverse all nodes which follow a node.
    ///
    /// If the rootref field of the seed node is empty all nodes will
    /// be traversed.
    /// 
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_head_follow_async(SignedHeadNode const & i_seed,
                                      SignedHeadTraverseFunc & i_func)
        throw(InternalError) = 0;

    /// Traverse only the extreme heads which follow a node.
    ///
    /// If the rootref field of the seed node is empty all extreme
    /// heads will be traversed.
    /// 
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_head_furthest_async(SignedHeadNode const & i_seed,
                                        SignedHeadTraverseFunc & i_func)
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
