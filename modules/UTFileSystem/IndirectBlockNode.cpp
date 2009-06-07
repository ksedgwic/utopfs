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
#include "IndirectBlockNode.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {


} // end namespace

namespace UTFS {

IndirectBlockNode::IndirectBlockNode()
{
    LOG(lgr, 6, "CTOR");
}

IndirectBlockNode::IndirectBlockNode(Context & i_ctxt,
                                     BlockRef const & i_ref)
{
    LOG(lgr, 6, "CTOR " << i_ref);

    uint8 * ptr = (uint8 *) m_blkref;
    size_t sz = BLKSZ;

    // Read the block from the blockstore.
    i_ctxt.m_bsh->bs_get_block(i_ref.data(), i_ref.size(), ptr, sz);

    // Validate the block.
    i_ref.validate(ptr, sz);

    // Decrypt the block.
    i_ctxt.m_cipher.decrypt(i_ref.iv(), ptr, sz);
}

IndirectBlockNode::~IndirectBlockNode()
{
    LOG(lgr, 6, "DTOR " << bn_blkref());
}

BlockRef
IndirectBlockNode::bn_persist(Context & i_ctxt)
{
    // Copy the data into a buffer.
    uint8 buf[BlockNode::BLKSZ];
    ACE_OS::memset(buf, '\0', BlockNode::BLKSZ);
    ACE_OS::memcpy(buf, m_blkref, sizeof(m_blkref));

    // Construct an initvec.
    utp::uint8 iv[16];
    Random::fill(iv, sizeof(iv));

    // Encrypt the entire block.
    i_ctxt.m_cipher.encrypt(iv, buf, sizeof(buf));

    // Set our reference value.
    m_ref = BlockRef(Digest(buf, sizeof(buf)), iv);

    LOG(lgr, 6, "persist " << bn_blkref());

    // Write the block out to the block store.
    i_ctxt.m_bsh->bs_put_block(m_ref.data(), m_ref.size(),
                               buf, sizeof(buf));

    return m_ref;
}

bool
IndirectBlockNode::rb_traverse(Context & i_ctxt,
                               FileNode & i_fn,
                               unsigned int i_flags,
                               off_t i_base,
                               off_t i_rngoff,
                               size_t i_rngsize,
                               BlockTraverseFunc & i_trav)
{
    RefBlockNode::BindingSeq mods;

    static size_t refspan = BLKSZ;

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
        DataBlockNodeHandle nh;

        // Do we have one in the cache already?
        if (m_blkobj[ndx])
        {
            // Yep, use it.
            nh = dynamic_cast<DataBlockNode *>(&*m_blkobj[ndx]);
        }
        else
        {
            // Nope, does it have a digest yet?
            if (m_blkref[ndx])
            {
                // Yes, read it from the blockstore.
                nh = new DataBlockNode(i_ctxt, m_blkref[ndx]);

                // Keep it in the cache.
                m_blkobj[ndx] = nh;
            }
            else if (i_flags & RB_MODIFY)
            {
                // Nope, create new block.
                nh = new DataBlockNode();

                // Keep it in the cache.
                m_blkobj[ndx] = nh;

                // Increment the block count.
                i_fn.blocks(i_fn.blocks() + 1);
            }
            else
            {
                // Use the zero singleton.
                nh = i_ctxt.m_zdatobj;

                // And *don't* keep it in the cache!
            }
        }

        // Visit it, deal with possible updates.
        if (i_trav.bt_visit(i_ctxt,
                            nh->bn_data(),
                            nh->bn_size(),
                            off,
                            i_fn.size()))
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
IndirectBlockNode::rb_update(Context & i_ctxt,
                             off_t i_base,
                             RefBlockNode::BindingSeq const & i_bs)
{
    size_t refspan = BLKSZ;

    for (unsigned i = 0; i < i_bs.size(); ++i)
    {
        size_t ndx = (i_bs[i].first - i_base) / refspan;
        m_blkref[ndx] = i_bs[i].second;
    }
}

ZeroIndirectBlockNode::ZeroIndirectBlockNode(DataBlockNodeHandle const & i_dbnh)
{
    LOG(lgr, 6, "CTOR " << "ZERO");

    // Initialize all of our references to the zero data block.
    for (unsigned i = 0; i < NUMREF; ++i)
        m_blkobj[i] = i_dbnh;
}


BlockRef
ZeroIndirectBlockNode::bn_persist(Context & i_ctxt)
{
    throwstream(InternalError, FILELINE
                << "persisting the zero indirect block makes me sad");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
