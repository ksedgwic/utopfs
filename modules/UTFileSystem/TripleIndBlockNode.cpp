#include "BlockCipher.h"
#include "BlockStore.h"
#include "Digest.h"
#include "Except.h"
#include "Log.h"
#include "Random.h"

#include "utfslog.h"

#include "BlockNodeCache.h"
#include "Context.h"
#include "DataBlockNode.h"
#include "FileNode.h"
#include "TripleIndBlockNode.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {


} // end namespace

namespace UTFS {

TripleIndBlockNode::TripleIndBlockNode()
{
    LOG(lgr, 6, "CTOR");
}

TripleIndBlockNode::TripleIndBlockNode(Context & i_ctxt,
                                     BlockRef const & i_ref)
    : IndirectBlockNode(i_ctxt, i_ref)
{
    LOG(lgr, 6, "CTOR " << i_ref);
}

TripleIndBlockNode::~TripleIndBlockNode()
{
    LOG(lgr, 6, "DTOR " << bn_blkref());
}

BlockRef const &
TripleIndBlockNode::bn_flush(Context & i_ctxt)
{
    // If we aren't dirty then we just return our current reference.
    if (!bn_isdirty())
        return bn_blkref();

    for (unsigned i = 0; i < NUMREF; ++i)
    {
        if (m_blkobj_X[i])
        {
            DoubleIndBlockNodeHandle nh =
                dynamic_cast<DoubleIndBlockNode *>(&*m_blkobj_X[i]);

            // Flush the object.
            m_blkref[i] = nh->bn_flush(i_ctxt);

            // Insert it in the clean cache.
            i_ctxt.m_bncachep->insert(nh);

            // Clear it in the dirty array.
            m_blkobj_X[i] = NULL;
        }
    }

    return bn_persist(i_ctxt);
}

bool
TripleIndBlockNode::rb_traverse(Context & i_ctxt,
                                FileNode & i_fn,
                                unsigned int i_flags,
                                off_t i_base,
                                off_t i_rngoff,
                                size_t i_rngsize,
                                BlockTraverseFunc & i_trav)
{
    static off_t const refspan = NUMREF * NUMREF * BLKSZ;

    off_t startoff = max(i_rngoff, i_base);

    // Are we beyond the target range?
    if (i_base > i_rngoff + off_t(i_rngsize))
        goto done;

    // Figure out which index we start with.
    for (off_t ndx = (startoff - i_base) / refspan; ndx < off_t(NUMREF); ++ndx)
    {
        off_t off = i_base + (ndx * refspan);

        // If we are beyond the traversal region we're done.
        if (off >= i_rngoff + off_t(i_rngsize))
            goto done;

        // Find the block object to use.
        DoubleIndBlockNodeHandle nh;

        // Do we have one in the cache already?
        if (m_blkobj_X[ndx])
        {
            // Yep, use it.
            nh = dynamic_cast<DoubleIndBlockNode *>(&*m_blkobj_X[ndx]);
        }
        else
        {
            // Nope, does it have a digest yet?
            if (m_blkref[ndx])
            {
                // Does the clean cache have it?
                BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_blkref[ndx]);
                if (bnh)
                {
                    // Yes, better be a DoubleIndBlockNode ...
                    nh = dynamic_cast<DoubleIndBlockNode *>(&*bnh);
                }
                else
                {
                    // Nope, read it from the blockstore.
                    nh = new DoubleIndBlockNode(i_ctxt, m_blkref[ndx]);

                    // Insert it in the clean cache.
                    i_ctxt.m_bncachep->insert(nh);
                }
            }
            else if (i_flags & RB_MODIFY)
            {
                // Nope, create new block.
                nh = new DoubleIndBlockNode();

                // Keep it in the dirty cache.
                m_blkobj_X[ndx] = nh;

                // Increment the block count.
                i_fn.blocks(i_fn.blocks() + 1);
            }
            else
            {
                // Use the zero singleton.
                nh = i_ctxt.m_zdinobj;

                // And *don't* keep it in the cache!
            }
        }

        // Recursively traverse ...
        if (nh->rb_traverse(i_ctxt, i_fn, i_flags, off,
                            i_rngoff, i_rngsize, i_trav))
        {
            // The node is already marked dirty ...

            // Remove it from the clean cache.
            i_ctxt.m_bncachep->remove(nh->bn_blkref());

            // Insert it in the dirty collection.
            m_blkobj_X[ndx] = nh;

            // We're dirty too.
            bn_isdirty(true);
        }
    }

 done:
    // Return our dirty state.
    return bn_isdirty();
}

size_t
TripleIndBlockNode::rb_truncate(Context & i_ctxt,
                                off_t i_base,
                                off_t i_size)
{
    static off_t const refspan = NUMREF * NUMREF * BLKSZ;

    size_t nblocks = 1;		// Start w/ this node.
    off_t off = i_base;

    for (unsigned ndx = 0; ndx < NUMREF; ++ndx)
    {
        // Is it possibly overlapping the live portion of the file?
        if (off < i_size)
        {
            // Yes, it's part of the live file.
            if (m_blkobj_X[ndx] || m_blkref[ndx])
            {
                DoubleIndBlockNodeHandle nh;
            
                // Do we have one in the cache already?
                if (m_blkobj_X[ndx])
                {
                    // Yep, use it.
                    nh = dynamic_cast<DoubleIndBlockNode *>(&*m_blkobj_X[ndx]);
                }
                else
                {
                    // Nope, does it have a digest yet?
                    if (m_blkref[ndx])
                    {
                        // Does the clean cache have it?
                        BlockNodeHandle bnh =
                            i_ctxt.m_bncachep->lookup(m_blkref[ndx]);
                        if (bnh)
                        {
                            // Yes, better be a DoubleIndBlockNode ...
                            nh = dynamic_cast<DoubleIndBlockNode *>(&*bnh);
                        }
                        else
                        {
                            // Nope, read it from the blockstore.
                            nh = new DoubleIndBlockNode(i_ctxt, m_blkref[ndx]);

                            // Insert it in the clean cache.
                            i_ctxt.m_bncachep->insert(nh);
                        }
                    }
                }

                // Traverse the child.
                size_t nb = nh->rb_truncate(i_ctxt, off, i_size);

                // If it's dirtied so are we ...
                if (nh->bn_isdirty())
                {
                    // Remove it from the clean cache.
                    i_ctxt.m_bncachep->remove(nh->bn_blkref());

                    // Insert it in the dirty collection.
                    m_blkobj_X[ndx] = nh;

                    // We're dirty too.
                    bn_isdirty(true);
                }

                // Did it have child blocks?  If not purge it ...
                if (nb > 1)
                {
                    // It had children, keep it.
                    nblocks += nb;
                }
                else
                {
                    // This seems redundant with the remove above, surely
                    // one is enough?

                    // Remove from the clean cache.
                    i_ctxt.m_bncachep->remove(m_blkref[ndx]);

                    m_blkref[ndx].clear();
                    m_blkobj_X[ndx] = NULL;
                    bn_isdirty(true);
                }
            }
        }
        else
        {
            // Nope, it's truncated.
            // We're just removing the references.

            // Remove from the clean cache.
            i_ctxt.m_bncachep->remove(m_blkref[ndx]);

            m_blkref[ndx].clear();
            m_blkobj_X[ndx] = NULL;
            bn_isdirty(true);
        }

        off += refspan;
    }

    return nblocks;
}

size_t
TripleIndBlockNode::rb_refresh(Context & i_ctxt, uint64 i_rid)
{
    size_t nblocks = 0;

    BlockStore::KeySeq keys;

    for (unsigned i = 0; i < NUMREF; ++i)
    {
        if (m_blkref[i])
        {
            keys.push_back(m_blkref[i]);
            ++nblocks;

            DoubleIndBlockNodeHandle nh;
            if (m_blkobj_X[i])
            {
                nh = dynamic_cast<DoubleIndBlockNode *>(&*m_blkobj_X[i]);
            }
            else
            {
                // Does the clean cache have it?
                BlockNodeHandle bnh = i_ctxt.m_bncachep->lookup(m_blkref[i]);
                if (bnh)
                {
                    // Yes, better be a DoubleIndBlockNode ...
                    nh = dynamic_cast<DoubleIndBlockNode *>(&*bnh);
                }
                else
                {
                    // Nope, read it from the blockstore.
                    nh = new DoubleIndBlockNode(i_ctxt, m_blkref[i]);

                    // Let's not insert refresh blocks it in the clean
                    // cache ...
                }
            }

            nblocks += nh->rb_refresh(i_ctxt, i_rid);
        }
    }

    BlockStore::KeySeq missing;
    i_ctxt.m_bsh->bs_refresh_blocks(i_rid, keys, missing);
    if (!missing.empty())
        throwstream(InternalError, FILELINE << "missing blocks encountered");

    return nblocks;
}

ZeroTripleIndBlockNode::ZeroTripleIndBlockNode(DoubleIndBlockNodeHandle const & i_nh)
{
    LOG(lgr, 6, "CTOR " << "ZERO");

    // Initialize all of our references to the zero data block.
    for (unsigned i = 0; i < NUMREF; ++i)
        m_blkobj_X[i] = i_nh;

    // We aren't ever dirty.
    m_isdirty = false;
}


BlockRef const &
ZeroTripleIndBlockNode::bn_persist(Context & i_ctxt)
{
    throwstream(InternalError, FILELINE
                << "persisting the zero triple indirect block makes me sad");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
