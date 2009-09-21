#include "BlockCipher.h"
#include "BlockStore.h"
#include "Digest.h"
#include "Except.h"
#include "Log.h"
#include "Random.h"

#include "utfslog.h"

#include "BlockNode.h"
#include "DataBlockNode.h"
#include "IndirectBlockNode.h"
#include "DoubleIndBlockNode.h"
#include "Context.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace UTFS {

BlockNode::BlockNode()
    : m_isdirty(true)
{
}

BlockNode::BlockNode(BlockRef const & i_ref)
  : m_ref(i_ref)
  , m_isdirty(false)
{
    LOG(lgr, 6, "CTOR " << i_ref);
}

BlockNode::~BlockNode()
{
    LOG(lgr, 6, "DTOR " << bn_blkref());
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
