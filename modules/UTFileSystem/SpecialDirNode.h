#ifndef UTFS_SpecialDirNode_h__
#define UTFS_SpecialDirNode_h__

/// @file SpecialDirNode.h
/// Utopia FileSystem Special Directory Node Object.
///
/// This object implements the special ".utopfs" directory.

#include <string>

#include "utpfwd.h"
#include "utfsexp.h"

#include "DirNode.h"

namespace UTFS {

class UTFS_EXP SpecialDirNode : public DirNode
{
public:
    SpecialDirNode();

    virtual ~SpecialDirNode();

    // Traverse a path.
    virtual void traverse(Context & i_ctxt,
                          unsigned int i_flags,
                          std::string const & i_entry,
                          std::string const & i_rmndr,
                          NodeTraverseFunc & i_trav);

    virtual int getattr(Context & i_ctxt,
                        struct stat * o_statbuf);

    virtual int open(Context & i_ctxt,
                     std::string const & i_entry,
                     int i_flags);

    virtual int readdir(Context & i_ctxt,
                        off_t i_offset,
                        utp::FileSystem::DirEntryFunc & o_entryfunc);

private:
    FileNodeHandle		m_version;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_SpecialDirNode_h__
