#include "Log.h"
#include "BlockStore.h"
#include "BlockStoreFactory.h"

#include "VBSChild.h"
#include "vbslog.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSChild::VBSChild(string const & i_instname)
    : m_instname(i_instname)
{
    LOG(lgr, 4, m_instname << ' ' << "CTOR");

    m_bsh = BlockStoreFactory::lookup(i_instname);
}

VBSChild::~VBSChild()
{
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
