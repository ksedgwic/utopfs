#ifndef UTFS_DirNode_h__
#define UTFS_DirNode_h__

/// @file DirNode.h
/// Utopia FileSystem Directory Node Object.
///
/// See README.txt for inheritance diagram.

#include <map>
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
    // Node traversal functor base class.
    //
    class UTFS_EXP NodeTraverseFunc
    {
    public:
        // Called on the leaf node of a traversal.  Empty by default.
        virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
        {}

        // Called on the parent node of a traversal.  The parent is
        // the second from the bottom, the parent of the leaf.  Empty
        // by default.
        //
        virtual void nt_parent(Context & i_ctxt,
                               DirNode & i_dn,
                               std::string const & i_entry)
        {}

        // Accesses the "return value" assigned by the leaf or parent
        // routine.
        //
        int nt_retval() const { return m_retval; }

    protected:
        // Called to set the return value in the leaf or parent
        // routine.
        //
        void nt_retval(int i_retval) { m_retval = i_retval; }

    private:
        int				m_retval;
    };

    // Split a path into the next component and the remainder.
    static std::pair<std::string, std::string>
        pathsplit(std::string const & i_path);

    // Default Constructor.
    DirNode(mode_t i_mode,
            std::string const & i_uname,
            std::string const & i_gname);

    // "Upgrade Constructor" from FileNode.
    DirNode(Context & i_ctxt, FileNode const & i_fn);

    // Constructor from blockstore persisted data.
    DirNode(Context & i_ctxt, BlockRef const & i_ref);

    // Destructor.
    virtual ~DirNode();

    // Traversal option flags.
    enum TraverseFlags
    {
        NT_DEFAULT		= 0x0,	// Traverse to leaf, don't update.
        NT_PARENT		= 0x1,	// Stop at parent, leaf may be missing.
        NT_UPDATE		= 0x2,	// Call the update method.
        NT_READONLY		= 0x4	// OperationError instead of modifying.
    };

    // Flush any dirty pages to the blockstore and return digest.
    virtual BlockRef const & bn_flush(Context & i_ctxt);

    virtual void bn_tostream(std::ostream & ostrm) const;

    virtual size_t rb_refresh(Context & i_ctxt, utp::uint64 i_rid);
                            
    // Traverse a path calling functor methods as appropriate.
    //
    // If NT_READONLY is set throws OperationError if it would
    // need to make any changes to the tree (even filling in nodes).
    //
    virtual void node_traverse(Context & i_ctxt,
                               unsigned int i_flags,
                               std::string const & i_entry,
                               std::string const & i_rmndr,
                               NodeTraverseFunc & i_trav);

    // Returns the number of entries in the directory.
    virtual size_t numentries() const;

    virtual int mknod(Context & i_ctxt,
                      std::string const & i_entry,
                      mode_t i_mode,
                      dev_t i_dev,
                      std::string const & i_uname,
                      std::string const & i_gname);

    virtual int mkdir(Context & i_ctxt,
                      std::string const & i_entry,
                      mode_t i_mode,
                      std::string const & i_uname,
                      std::string const & i_gname);

    virtual int unlink(Context & i_ctxt,
                       std::string const & i_entry,
                       bool i_dirstoo);

    virtual int rmdir(Context & i_ctxt,
                      std::string const & i_entry);

    virtual int symlink(Context & i_ctxt,
                        std::string const & i_entry,
                        std::string const & i_opath);

    virtual BlockRef linksrc(Context & i_ctxt,
                             std::string const & i_entry);

    virtual int linkdst(Context & i_ctxt,
                        std::string const & i_entry,
                        BlockRef const & i_blkref,
                        bool i_force);

    virtual int open(Context & i_ctxt,
                     std::string const & i_entry,
                     int i_flags);

    virtual int readdir(Context & i_ctxt,
                        off_t i_offset,
                        utp::FileSystem::DirEntryFunc & o_entryfunc);

protected:
    virtual void update(Context & i_ctxt,
                        std::string const & i_entry,
                        FileNodeHandle const & i_fnh);

    virtual FileNodeHandle lookup(Context & i_ctxt,
                                  std::string const & i_entry,
                                  bool i_readonly);

    virtual BlockRef find_blkref(Context & i_ctxt,
                                 std::string const & i_entry);

    void deserialize(Context & i_ctxt);

    void update(std::string const & i_entry, BlockRef const & i_blkref);

    void remove(Context & i_ctxt, std::string const & i_entry);

private:
    typedef std::map<std::string, FileNodeHandle> EntryMap;

    // Name to digest mapppings (what is persisted).
    Directory				m_dir;

    // Name to dirty FileNodeHandle mappings (cached).
    EntryMap				m_dirty;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_DirNode_h__
