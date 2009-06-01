#include "Log.h"
#include "Except.h"

#include "BlockStore.h"
#include "StreamCipher.h"

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

DataBlockNode::DataBlockNode(Context & i_ctxt, utp::Digest const & i_dig)
{
    throwstream(InternalError, FILELINE
                << "DataBlockNode::DataBlockNode unimplemented");
}

DataBlockNode::~DataBlockNode()
{
}

void
DataBlockNode::bn_persist(Context & i_ctxt)
{
    // FIXME - Totally bogus, zero an initvec.
    utp::uint8 initvec[8];
    ACE_OS::memset(initvec, '\0', sizeof(initvec));

    // Encrypt the entire block.
    i_ctxt.m_cipher.encrypt(initvec, 0, m_data, sizeof(m_data));

    // Take the digest of the whole thing.
    bn_digest(Digest(m_data, sizeof(m_data)));

    LOG(lgr, 6, "persist " << bn_digest());

    // Write the block out to the block store.
    i_ctxt.m_bsh->bs_put_block(bn_digest().data(), bn_digest().size(),
                               m_data, sizeof(m_data));
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
                                     utp::Digest const & i_dig)
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
