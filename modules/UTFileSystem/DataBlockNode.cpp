#include "BlockCipher.h"
#include "BlockStore.h"
#include "Digest.h"
#include "Except.h"
#include "Log.h"
#include "Random.h"

#include "utfslog.h"

#include "Context.h"
#include "DataBlockNode.h"
#include "DoubleIndBlockNode.h"
#include "IndirectBlockNode.h"
#include "UTFileSystem.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {


} // end namespace

namespace UTFS {

DataBlockNode::DataBlockNode()
{
    LOG(lgr, 6, "CTOR");

    // Clear all the memory in the block.
    ACE_OS::memset(m_data, '\0', sizeof(m_data));
}

DataBlockNode::DataBlockNode(Context & i_ctxt, BlockRef const & i_ref)
    : BlockNode(i_ref)
{
    LOG(lgr, 6, "CTOR " << i_ref);

    size_t sz = sizeof(m_data);

    // Read the block from the blockstore.
    i_ctxt.m_bsh->bs_block_get(i_ref.data(), i_ref.size(), m_data, sz);

    ++i_ctxt.m_statsp->m_ngops;
    i_ctxt.m_statsp->m_ngbytes += sz;;

    // Validate the block.
    i_ref.validate(m_data, sz);

    // Decrypt the block.
    i_ctxt.m_cipher.decrypt(i_ref.iv(), m_data, sz);
}

DataBlockNode::~DataBlockNode()
{
    LOG(lgr, 6, "DTOR " << bn_blkref());
}

BlockRef const &
DataBlockNode::bn_persist(Context & i_ctxt)
{
    // Copy the data into a buffer.
    uint8 buf[sizeof(m_data)];
    ACE_OS::memcpy(buf, m_data, sizeof(m_data));

    // Construct an initvec.
    utp::uint8 iv[16];
    Random::fill(iv, sizeof(iv));

    // Encrypt the entire block.
    i_ctxt.m_cipher.encrypt(iv, buf, sizeof(buf));

    // Set our reference value.
    m_ref = BlockRef(Digest(buf, sizeof(buf)), iv);

    LOG(lgr, 6, "persist " << bn_blkref());

    // Write the block out to the block store.
    i_ctxt.m_bsh->bs_block_put((void *) m_ref.data(),
                               m_ref.size(),
                               (void *) buf,
                               sizeof(buf));

    ++i_ctxt.m_statsp->m_npops;
    i_ctxt.m_statsp->m_npbytes += sizeof(buf);

    bn_isdirty(false);

    return m_ref;
}

BlockRef const &
DataBlockNode::bn_flush(Context & i_ctxt)
{
    return bn_persist(i_ctxt);
}

void
DataBlockNode::bn_tostream(std::ostream & ostrm) const
{
    BlockNode::bn_tostream(ostrm);
    ostrm << ' ' << "DataBlockNode";
}

ZeroDataBlockNode::ZeroDataBlockNode()
{
    LOG(lgr, 6, "CTOR " << "ZERO");

    // We aren't ever dirty.
    m_isdirty = false;
}

#if 0
long
ZeroDataBlockNode::rc_add_ref(void * ptr) const
{
    // Breakpoint here to see new references ...
    return BlockNode::rc_add_ref(ptr);
}
#endif

void
ZeroDataBlockNode::bn_isdirty(bool i_isdirty)
{
    if (i_isdirty)
        throwstream(InternalError, FILELINE
                    << "dirtying the zero data block makes me sad");

    // Delegate actual work.
    DataBlockNode::bn_isdirty(i_isdirty);
}

BlockRef const &
ZeroDataBlockNode::bn_persist(Context & i_ctxt)
{
    throwstream(InternalError, FILELINE
                << "persisting the zero data block makes me sad");
}

void
ZeroDataBlockNode::bn_tostream(std::ostream & ostrm) const
{
    BlockNode::bn_tostream(ostrm);
    ostrm << ' ' << "ZeroDataBlockNode";
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
