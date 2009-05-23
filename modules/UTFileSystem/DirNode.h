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

class UTFS_EXP DirNode : public FileNode
{
public:
    DirNode();

    virtual ~DirNode();

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
    // Split a path into the next component and the remainder.
    static std::pair<std::string, std::string>
        pathsplit(std::string const & i_path);

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
