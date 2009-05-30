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

BlockNode::~BlockNode()
{
    LOG(lgr, 6, "DTOR");
}

DataBlockNode::DataBlockNode()
{
    throwstream(InternalError, FILELINE
                << "DataBlockNode::DataBlockNode unimplemented");
}

DataBlockNode::DataBlockNode(Context & i_ctxt, utp::Digest const & i_dig)
{
    throwstream(InternalError, FILELINE
                << "DataBlockNode::DataBlockNode unimplemented");
}

DataBlockNode::~DataBlockNode()
{
    throwstream(InternalError, FILELINE
                << "DataBlockNode::~DataBlockNode unimplemented");
}

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
    throwstream(InternalError, FILELINE
                << "IndirectBlockNode::~IndirectBlockNode unimplemented");
}

void
IndirectBlockNode::update(Context & i_ctxt, BindingSeq const & i_bs)
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
