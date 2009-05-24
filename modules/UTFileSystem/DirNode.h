#ifndef UTFS_DirNode_h__
#define UTFS_DirNode_h__

/// @file DirNode.h
/// Utopia FileSystem Directory Node Object.

#include <string>

#include "FileSystem.h"

#include "Directory.pb.h"

#include "utfsfwd.h"
#include "utfsexp.h"

#include "FileNode.h"

namespace UTFS {

class UTFS_EXP TraverseFunc
{
public:
    // Called on the leaf node of a traversal.
    virtual void tf_leaf(FileNode & i_fn)
    {}

    // Called on the parent node of a traversal.
    virtual void tf_parent(DirNode & i_dn, std::string const & i_entry)
    {}

    // Called post-traversal on directories for updating.
    virtual void tf_update(DirNode & i_dn,
                           std::string const & i_entry,
                           utp::Digest const & i_dig)
    {}

    int retval() const { return m_retval; }

protected:
    void retval(int i_retval) { m_retval = i_retval; }

private:
    int				m_retval;
};

class UTFS_EXP DirNode : public FileNode
{
public:
    // Split a path into the next component and the remainder.
    static std::pair<std::string, std::string>
        pathsplit(std::string const & i_path);

    DirNode();

    virtual ~DirNode();

    // Traverse a path.
    virtual void traverse(std::string const & i_entry,
                          std::string const & i_rmndr,
                          TraverseFunc & i_trav);

    // Resolves a path into a file node object.
    virtual FileNodeHandle resolve(std::string const & i_path);

    // Resolves a path into a parent node object and a remainder path.
    virtual std::pair<DirNodeHandle, std::string>
        resolve_parent(std::string const & i_path);

    virtual int getattr(struct stat * o_statbuf);

    virtual int open(std::string const & i_entry, int i_flags);

    virtual int readdir(off_t i_offset,
                        utp::FileSystem::DirEntryFunc & o_entryfunc);

protected:

private:
    Directory				m_dir;
    
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_DirNode_h__
