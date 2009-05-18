#include "Log.h"

#include "utfslog.h"

#include "SpecialFileNode.h"

using namespace std;
using namespace utp;

namespace UTFS {

SpecialFileNode::SpecialFileNode(string const & i_data)
    : m_data(i_data)
{
    LOG(lgr, 4, "CTOR");
}

SpecialFileNode::~SpecialFileNode()
{
    LOG(lgr, 4, "DTOR");
}

int
SpecialFileNode::getattr(struct stat * o_stbuf)
{
    o_stbuf->st_mode = S_IFREG | 0444;
    o_stbuf->st_nlink = 1;
    o_stbuf->st_size = m_data.size();

    return 0;
}

int
SpecialFileNode::read(void * o_bufptr, size_t i_size, off_t i_off)
{
    off_t len = m_data.size();
    if (i_off < len)
    {
        if (i_off + off_t(i_size) > len)
            i_size = len - i_off;
        memcpy(o_bufptr, &m_data[i_off], i_size);
    }
    else
    {
        i_size = 0;
    }
    return int(i_size);
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
