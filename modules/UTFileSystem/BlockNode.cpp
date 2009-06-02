#include "BlockCipher.h"
#include "BlockStore.h"
#include "Digest.h"
#include "Except.h"
#include "Log.h"
#include "Random.h"

#include "utfslog.h"

#include "BlockNode.h"
#include "Context.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {


} // end namespace

namespace UTFS {

// ----------------------------------------------------------------
// BlockNode methods
// ----------------------------------------------------------------

BlockNode::~BlockNode()
{
    LOG(lgr, 6, "DTOR");
}

// ----------------------------------------------------------------
// DataBlockNode methods
// ----------------------------------------------------------------

DataBlockNode::DataBlockNode()
{
    // Clear all the memory in the block.
    ACE_OS::memset(m_data, '\0', sizeof(m_data));
}

DataBlockNode::DataBlockNode(Context & i_ctxt, BlockRef const & i_ref)
{
    throwstream(InternalError, FILELINE
                << "DataBlockNode::DataBlockNode unimplemented");
}

DataBlockNode::~DataBlockNode()
{
}

BlockRef
DataBlockNode::bn_persist(Context & i_ctxt)
{
    // Construct an initvec.
    utp::uint8 iv[8];
    Random::fill(iv, sizeof(iv));

    // Encrypt the entire block.
    i_ctxt.m_cipher.encrypt(iv, m_data, sizeof(m_data));

    // Set our reference value.
    m_ref = BlockRef(Digest(m_data, sizeof(m_data)), iv);

    LOG(lgr, 6, "persist " << bn_blkref());

    // Write the block out to the block store.
    i_ctxt.m_bsh->bs_put_block(bn_blkref().data(), bn_blkref().size(),
                               m_data, sizeof(m_data));

    return m_ref;
}

// ----------------------------------------------------------------
// RefBlockNode methods
// ----------------------------------------------------------------

void
RefBlockNode::BlockTraverseFunc::bt_update(Context & i_ctxt,
                                           RefBlockNode & i_bn,
                                           BindingSeq const & i_bbs)
{
    i_bn.rb_update(i_ctxt, i_bbs);
}

RefBlockNode::~RefBlockNode()
{
}

// ----------------------------------------------------------------
// IndirectBlockNode methods
// ----------------------------------------------------------------

IndirectBlockNode::IndirectBlockNode()
{
    throwstream(InternalError, FILELINE
                << "IndirectBlockNode::IndirectBlockNode unimplemented");
}

IndirectBlockNode::IndirectBlockNode(Context & i_ctxt,
                                     BlockRef const & i_ref)
{
    throwstream(InternalError, FILELINE
                << "IndirectBlockNode::IndirectBlockNode unimplemented");
}

IndirectBlockNode::~IndirectBlockNode()
{
}

void
IndirectBlockNode::rb_update(Context & i_ctxt,
                             RefBlockNode::BindingSeq const & i_bs)
{
    throwstream(InternalError, FILELINE
                << "IndirectBlockNode::update unimplemented");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
