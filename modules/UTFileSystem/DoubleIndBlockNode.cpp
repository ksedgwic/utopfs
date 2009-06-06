#include "BlockCipher.h"
#include "BlockStore.h"
#include "Digest.h"
#include "Except.h"
#include "Log.h"
#include "Random.h"

#include "utfslog.h"

#include "Context.h"
#include "DataBlockNode.h"
#include "FileNode.h"
#include "DoubleIndBlockNode.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {


} // end namespace

namespace UTFS {

DoubleIndBlockNode::DoubleIndBlockNode()
{
    LOG(lgr, 6, "CTOR");
}

DoubleIndBlockNode::DoubleIndBlockNode(Context & i_ctxt,
                                     BlockRef const & i_ref)
    : IndirectBlockNode(i_ctxt, i_ref)
{
    LOG(lgr, 6, "CTOR " << i_ref);
}

DoubleIndBlockNode::~DoubleIndBlockNode()
{
    LOG(lgr, 6, "DTOR");
}

bool
DoubleIndBlockNode::rb_traverse(Context & i_ctxt,
                               FileNode & i_fn,
                               unsigned int i_flags,
                               off_t i_base,
                               off_t i_rngoff,
                               size_t i_rngsize,
                               BlockTraverseFunc & i_trav)
{
    RefBlockNode::BindingSeq mods;

    static size_t refspan = NUMREF * BLKSZ;

    // Are we beyond the target range?
    if (i_base > i_rngoff + off_t(i_rngsize))
        goto done;

    // Figure out which index we start with.
    for (off_t ndx = (i_rngoff - i_base) / refspan; ndx < off_t(NUMREF); ++ndx)
    {
        off_t off = i_base + (ndx * refspan);

        // If we are beyond the traversal region we're done.
        if (off > i_rngoff + off_t(i_rngsize))
            goto done;

        // Find the block object to use.
        IndirectBlockNodeHandle nh;

        // Do we have one in the cache already?
        if (m_blkobj[ndx])
        {
            // Yep, use it.
            nh = dynamic_cast<IndirectBlockNode *>(&*m_blkobj[ndx]);
        }
        else
        {
            // Nope, does it have a digest yet?
            if (m_blkref[ndx])
            {
                // Yes, read it from the blockstore.
                nh = new IndirectBlockNode(i_ctxt, m_blkref[ndx]);

                // Keep it in the cache.
                m_blkobj[ndx] = nh;
            }
            else if (i_flags & RB_MODIFY)
            {
                // Nope, create new block.
                nh = new IndirectBlockNode();

                // Keep it in the cache.
                m_blkobj[ndx] = nh;

                // Increment the block count.
                i_fn.blocks(i_fn.blocks() + 1);
            }
            else
            {
                // Use the zero singleton.
                nh = i_ctxt.m_zsinobj;

                // And *don't* keep it in the cache!
            }
        }

        // Recursively traverse ...
        if (nh->rb_traverse(i_ctxt, i_fn, i_flags, off,
                            i_rngoff, i_rngsize, i_trav))
        {
            nh->bn_persist(i_ctxt);
            mods.push_back(make_pair(off, nh->bn_blkref()));
        }
    }

 done:
    // Were there any modified regions?
    if (mods.empty())
    {
        return false;
    }
    else
    {
        i_trav.bt_update(i_ctxt, *this, i_base, mods);
        return true;
    }
}

void
DoubleIndBlockNode::rb_update(Context & i_ctxt,
                             off_t i_base,
                             RefBlockNode::BindingSeq const & i_bs)
{
    size_t refspan = NUMREF * BLKSZ;

    for (unsigned i = 0; i < i_bs.size(); ++i)
    {
        size_t ndx = (i_bs[i].first - i_base) / refspan;
        m_blkref[ndx] = i_bs[i].second;
    }
}

ZeroDoubleIndBlockNode::ZeroDoubleIndBlockNode(IndirectBlockNodeHandle const & i_nh)
{
    LOG(lgr, 6, "CTOR");

    // Initialize all of our references to the zero data block.
    for (unsigned i = 0; i < NUMREF; ++i)
        m_blkobj[i] = i_nh;
}


BlockRef
ZeroDoubleIndBlockNode::bn_persist(Context & i_ctxt)
{
    throwstream(InternalError, FILELINE
                << "persisting the zero double indirect block makes me sad");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
