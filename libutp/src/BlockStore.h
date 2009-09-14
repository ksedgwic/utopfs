#ifndef utp_BlockStore_h__
#define utp_BlockStore_h__

/// @file BlockStore.h
/// Abstract BlockStore Interface.

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "utpexp.h"
#include "utpfwd.h"

#include "HeadEdge.pb.h"
#include "Except.h"
#include "RC.h"
#include "Types.h"

namespace utp {

/// HeadNode reference (binary fstag, binary encrypted root BlockRef)
typedef std::pair<std::string, std::string> HeadNode;

/// Sequence of HeadNode objects.
typedef std::vector<HeadNode> HeadNodeSeq;

/// Set of HeadNode objects.
typedef std::set<HeadNode> HeadNodeSet;

// Helpful for debugging.
std::ostream & operator<<(std::ostream & ostrm, HeadNode const & i_hn);

// Crutch for some namespace complexities.
std::string mkstring(HeadNode const & i_hn);

class UTP_EXP BlockStore : public virtual RCObj
{
public:
    /// Sequence of keys.
    typedef std::vector<OctetSeq> KeySeq;

    /// Sequence of Signed Head Edges
    typedef std::deque<SignedHeadEdge> SignedHeadEdgeSeq;

    /// Destructor
    ///
    virtual ~BlockStore();

    /// Returns instance name
    virtual std::string const & bs_instname() const = 0;
    
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
    /// @throw ValueError One of the arguments is out of range.
    ///
    virtual void bs_open(StringSeq const & i_args)
        throw(InternalError,
              NotFoundError,
              ValueError) = 0;

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

    /// Ensures blocks are persisted.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_sync()
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
    size_t bs_block_get(void const * i_keydata,
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
    void bs_block_put(void const * i_keydata,
                      size_t i_keysize,
                      void const * i_blkdata,
                      size_t i_blksize)
        throw(InternalError,
              ValueError,
              NoSpaceError);

    /// Start a refresh cycle.
    ///
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_rid A unique refresh cycle identifier.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotUniqueError The specified refresh id is already in use.
    ///
    void bs_refresh_start(utp::uint64 i_rid)
        throw(InternalError,
              NotUniqueError);

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
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_rid The unique refresh cycle identifier.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified refresh id was not found.
    ///
    void bs_refresh_finish(utp::uint64 i_rid)
        throw(InternalError,
              NotFoundError);

    /// Insert a SignedHeadEdge into the BlockStore.
    ///
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_she The SignedHeadEdge
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    void bs_head_insert(SignedHeadEdge const & i_she)
        throw(InternalError);

    /// Return all edges which follow a node.
    ///
    /// If the seed node is empty all nodes will be traversed.
    ///
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_hn The seed head node.
    /// @param[out] o_edges Collection to hold returned edges.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError No edges were found.
    ///
    void bs_head_follow(HeadNode const & i_hn,
                        SignedHeadEdgeSeq & o_edges)
        throw(InternalError,
              NotFoundError);

    /// Return only the extreme nodes which follow a node.
    ///
    /// If the seed node is empty all extreme heads will be traversed.
    /// 
    /// @note This interface is a base class implemented wrapper on
    ///       the nonblocking interface.  Implementing classes do not
    ///       need to implement this ...
    ///
    /// @param[in] i_hn The seed head node.
    /// @param[out] o_nodes Collection to hold returned nodes.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError No nodes were found.
    ///
    void bs_head_furthest(HeadNode const & i_hn,
                          HeadNodeSeq & o_nodes)
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
                                 void const * i_argp,
                                 size_t i_blksize) = 0;

        virtual void bg_error(void const * i_keydata,
                              size_t i_keysize,
                              void const * i_argp,
                              Exception const & i_exp) = 0;
    };

    /// Get block via non-blocking interface.
    ///
     virtual void bs_block_get_async(void const * i_keydata,
                                    size_t i_keysize,
                                    void * o_buffdata,
                                    size_t i_buffsize,
                                    BlockGetCompletion & i_cmpl,
                                    void const * i_argp)
        throw(InternalError,
              ValueError) = 0;

    /// Completion function interface for non-blocking put.
    ///
    class BlockPutCompletion
    {
    public:
        virtual void bp_complete(void const * i_keydata,
                                 size_t i_keysize,
                                 void const * i_argp) = 0;

        virtual void bp_error(void const * i_keydata,
                              size_t i_keysize,
                              void const * i_argp,
                              Exception const & i_exp) = 0;
    };

    /// Put block via non-blocking interface.
    ///
    virtual void bs_block_put_async(void const * i_keydata,
                                    size_t i_keysize,
                                    void const * i_blkdata,
                                    size_t i_blksize,
                                    BlockPutCompletion & i_cmpl,
                                    void const * i_argp)
        throw(InternalError,
              ValueError) = 0;

    /// Completion function interface for async RefreshStart
    ///
    class RefreshStartCompletion
    {
    public:
        virtual void rs_complete(utp::uint64 i_rid,
                                 void const * i_argp) = 0;

        virtual void rs_error(utp::uint64 i_rid,
                              void const * i_argp,
                              Exception const & i_exp) = 0;
    };

    /// Start a refresh cycle via the non-blocking interface.
    ///
    virtual void bs_refresh_start_async(utp::uint64 i_rid,
                                        RefreshStartCompletion & i_cmpl,
                                        void const * i_argp)
        throw(InternalError) = 0;

    /// Completion function interface for non-blocking refresh.
    ///
    class RefreshBlockCompletion
    {
    public:
        virtual void rb_complete(void const * i_keydata,
                                 size_t i_keysize,
                                 void const * i_argp) = 0;

        virtual void rb_missing(void const * i_keydata,
                                size_t i_keysize,
                                void const * i_argp) = 0;
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
                                        RefreshBlockCompletion & i_cmpl,
                                        void const * i_argp)
        throw(InternalError,
              NotFoundError) = 0;

    /// Completion function interface for async RefreshFinish
    ///
    class RefreshFinishCompletion
    {
    public:
        virtual void rf_complete(utp::uint64 i_rid,
                                 void const * i_argp) = 0;

        virtual void rf_error(utp::uint64 i_rid,
                              void const * i_argp,
                              Exception const & i_exp) = 0;
    };

    /// Finish a refresh cycle via the non-blocking interface.
    ///
    virtual void bs_refresh_finish_async(utp::uint64 i_rid,
                                         RefreshFinishCompletion & i_cmpl,
                                         void const * i_argp)
        throw(InternalError) = 0;

    /// Completion callback interface for async SignedHeadEdge
    /// insertion.
    ///
    class HeadEdgeInsertCompletion
    {
    public:
        virtual void hei_complete(SignedHeadEdge const & i_she,
                                  void const * i_argp) = 0;

        virtual void hei_error(SignedHeadEdge const & i_she,
                               void const * i_argp,
                               Exception const & i_exp) = 0;
    };

    /// Insert a SignedHeadEdge into the BlockStore.
    ///
    /// @param[in] i_she The SignedHeadEdge to be inserted.
    /// @param[in] i_cmpl Completion function called when done.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_head_insert_async(SignedHeadEdge const & i_she,
                                      HeadEdgeInsertCompletion & i_cmpl,
                                      void const * i_argp)
        throw(InternalError) = 0;

    /// Callback interface for the follow tarversal.
    ///
    class HeadEdgeTraverseFunc
    {
    public:
        // Called on each edge in the traversal.
        virtual void het_edge(void const * i_argp,
                              SignedHeadEdge const & i_she) = 0;

        // Called when the traversal is complete.
        virtual void het_complete(void const * i_argp) = 0;

        // Called instead if there is an error.
        virtual void het_error(void const * i_argp,
                               Exception const & i_exp) = 0;
    };

    /// Traverse all edges which follow a node.
    ///
    /// If the rootref field of the seed node is empty all edges will
    /// be traversed.
    /// 
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_head_follow_async(HeadNode const & i_hn,
                                      HeadEdgeTraverseFunc & i_func,
                                      void const * i_argp)
        throw(InternalError) = 0;

    /// Callback interface for the furthest traversal.
    ///
    class HeadNodeTraverseFunc
    {
    public:
        // Called on each node in the traversal.
        virtual void hnt_node(void const * i_argp,
                              HeadNode const & i_hn) = 0;

        // Called when the traversal is complete.
        virtual void hnt_complete(void const * i_argp) = 0;

        // Called instead if there is an error.
        virtual void hnt_error(void const * i_argp,
                               Exception const & i_exp) = 0;
    };

    /// Traverse only the extreme heads which follow a node.
    ///
    /// If the rootref field of the seed node is empty all extreme
    /// heads will be traversed.
    /// 
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual void bs_head_furthest_async(HeadNode const & i_hn,
                                        HeadNodeTraverseFunc & i_func,
                                        void const * i_argp)
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
