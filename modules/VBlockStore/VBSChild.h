#ifndef VBSChild_h__
#define VBSChild_h__

/// @file VBSChild.h
/// Virtual BlockStore VBSChild

#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "RC.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

// Virtual BlockStore VBSChild
//
class VBS_EXP VBSChild : public utp::RCObj
{
public:
    VBSChild(std::string const & i_instname);

    virtual ~VBSChild();

protected:

private:
    std::string				m_instname;
    utp::BlockStoreHandle	m_bsh;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSChild_h__
