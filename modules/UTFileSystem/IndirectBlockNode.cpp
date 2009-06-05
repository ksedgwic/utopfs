#include "BlockCipher.h"
#include "BlockStore.h"
#include "Digest.h"
#include "Except.h"
#include "Log.h"
#include "Random.h"

#include "utfslog.h"

#include "IndirectBlockNode.h"
#include "Context.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {


} // end namespace

namespace UTFS {

IndirectBlockNode::IndirectBlockNode()
{
    ACE_OS::memset(m_reftbl, '\0', sizeof(m_reftbl));

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

BlockRef
IndirectBlockNode::bn_persist(Context & i_ctxt)
{
    // Copy the data into a buffer.
    uint8 buf[BlockNode::BLKSZ];
    ACE_OS::memset(buf, '\0', BlockNode::BLKSZ);
    ACE_OS::memcpy(buf, m_reftbl, sizeof(m_reftbl));

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
                               unsigned int i_flags,
                               off_t i_base,
                               off_t i_rngoff,
                               size_t i_rngsize,
                               BlockTraverseFunc & i_trav)
{
    throwstream(InternalError, FILELINE
                << "IndirectBlockNode::rb_traverse unimplemented");
}

void
IndirectBlockNode::rb_update(Context & i_ctxt,
                             RefBlockNode::BindingSeq const & i_bs)
{
    throwstream(InternalError, FILELINE
                << "IndirectBlockNode::rb_update unimplemented");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
