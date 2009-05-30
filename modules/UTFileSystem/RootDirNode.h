#ifndef UTFS_RootDirNode_h__
#define UTFS_RootDirNode_h__

/// @file RootDirNode.h
/// Utopia FileSystem Root Directory Node Object.

#include <string>

#include "utpfwd.h"
#include "utfsexp.h"

#include "DirNode.h"

namespace UTFS {

class UTFS_EXP RootDirNode : public DirNode
{
public:
    RootDirNode();

    RootDirNode(Context & i_ctxt, utp::Digest const & i_dig);

    virtual ~RootDirNode();

    // Traverse a path.
    virtual void traverse(Context & i_ctxt,
                          unsigned int i_flags,
                          std::string const & i_entry,
                          std::string const & i_rmndr,
                          NodeTraverseFunc & i_trav);

    virtual int getattr(Context & i_ctxt,
                        struct stat * o_statbuf);

    virtual int readdir(Context & i_ctxt,
                        off_t i_offset,
                        utp::FileSystem::DirEntryFunc & o_entryfunc);

private:
    DirNodeHandle			m_sdh;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_RootDirNode_h__
