#include "BlockCipher.h"
#include "BlockStore.h"
#include "Digest.h"
#include "Except.h"
#include "Log.h"
#include "Random.h"

#include "utfslog.h"

#include "RefBlockNode.h"
#include "Context.h"

using namespace std;
using namespace utp;
using namespace google::protobuf::io;

namespace {


} // end namespace

namespace UTFS {

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

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
