#include "utfslog.h"

#include "Context.h"
#include "DirNode.h"
#include "FileNode.h"

using namespace std;
using namespace utp;

namespace UTFS {

Context::Context()
    : m_utfsp(NULL)
    , m_ctxtcond(m_ctxtmutex)
    , m_waiters(false)
    , m_putsout(0)
{
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
