#ifndef UTFS_SpecialFileNode_h__
#define UTFS_SpecialFileNode_h__

/// @file SpecialFileNode.h
/// Utopia FileSystem Special File Node Object.
///
/// This object implementst the special files in the ".utopfs" directory.
///
/// See README.txt for inheritance diagram.

#include <string>

#include "utpfwd.h"
#include "utfsexp.h"

#include "FileNode.h"

namespace UTFS {

class UTFS_EXP SpecialFileNode : public FileNode
{
public:
    SpecialFileNode(std::string const & i_data);

    virtual ~SpecialFileNode();

    virtual int getattr(Context & i_ctxt,
                        struct statstb * o_statbuf);

    virtual int read(Context & i_ctxt,
                     void * o_bufptr,
                     size_t i_size,
                     off_t i_off,
                     unsigned int flags);

private:
    std::string				m_data;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_SpecialFileNode_h__
