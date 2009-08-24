#ifndef VBSRequestHolder_h__
#define VBSRequestHolder_h__

/// @file VBSRequestHolder.h
/// Virtual BlockStore RequestHolder Interface

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

class VBS_EXP VBSRequestHolder
{
public:
    virtual void rh_insert(VBSRequestHandle const & i_rh) = 0;

    virtual void rh_remove(VBSRequestHandle const & i_rh) = 0;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBSPutRequest_h__
