#ifndef UTFS_DirNode_h__
#define UTFS_DirNode_h__

/// @file DirNode.h
/// Utopia FileSystem Directory Node Object.

#include <map>
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
    virtual void tf_leaf(Context & i_ctxt, FileNode & i_fn)
    {}

    // Called on the parent node of a traversal.
    virtual void tf_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           std::string const & i_entry)
    {}

    // Called post-traversal on directories for updating.
    virtual void tf_update(Context & i_ctxt,
                           DirNode & i_dn,
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

    DirNode(FileNode const & i_fn);

    DirNode(Context & i_ctxt, utp::Digest const & i_dig);

    virtual ~DirNode();

    enum TraverseFlags
    {
        TF_DEFAULT		= 0x0,	// Traverse to leaf, don't update.
        TF_PARENT		= 0x1,	// Stop at parent, leaf may be missing.
        TF_UPDATE		= 0x2	// Call the update method.
    };

    // Traverse a path calling functor methods as appropriate.
    virtual void traverse(Context & i_ctxt,
                          unsigned int i_flags,
                          std::string const & i_entry,
                          std::string const & i_rmndr,
                          TraverseFunc & i_trav);

    // Persist this DirNode to blockstore and update digest.
    virtual void persist(Context & i_ctxt);

    // Update this entries digest in this directory and persist.
    virtual void update(Context & i_ctxt,
                        std::string const & i_entry,
                        utp::Digest const & i_dig);

    virtual int mknod(Context & i_ctxt,
                      std::string const & i_entry,
                      mode_t i_mode,
                      dev_t i_dev);

    virtual int mkdir(Context & i_ctxt,
                      std::string const & i_entry,
                      mode_t i_mode);

    virtual int open(Context & i_ctxt,
                     std::string const & i_entry,
                     int i_flags);

    virtual int readdir(Context & i_ctxt,
                        off_t i_offset,
                        utp::FileSystem::DirEntryFunc & o_entryfunc);

protected:
    virtual FileNodeHandle lookup(Context & i_ctxt,
                                  std::string const & i_entry);

    void deserialize();

private:
    typedef std::map<std::string, FileNodeHandle> EntryMap;

    // Name to digest mapppings (what is persisted).
    Directory				m_dir;

    // Name to FileNodeHandle mappings (cached).
    EntryMap				m_cache;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_DirNode_h__
