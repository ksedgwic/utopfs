#include <sstream>
#include <string>

#include <ace/Guard_T.h>

#include "Base32.h"
#include "BlockStoreFactory.h"
#include "Log.h"

#include "utfslog.h"

#include "DirNode.h"
#include "FileNode.h"
#include "RootDirNode.h"
#include "UTFileSystem.h"

using namespace std;
using namespace utp;

namespace UTFS {

UTFileSystem::UTFileSystem()
{
    LOG(lgr, 4, "CTOR");
}

UTFileSystem::~UTFileSystem()
{
    // Don't try and log here ... in static object destructor context
    // (way after main has returned ...)
}

void
UTFileSystem::fs_mkfs(BlockStoreHandle const & i_bsh,
                      string const & i_fsid,
                      string const & i_passphrase,
                      StringSeq const & i_args)
    throw (InternalError,
           ValueError)
{
    LOG(lgr, 4, "fs_mkfs " << i_fsid);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    m_ctxt.m_bsh = i_bsh;

    // Save the digest of the fsid.
    m_fsiddig = Digest(i_fsid.data(), i_fsid.size());

    // Use part of the digest of the passphrase as the key.
    Digest dig(i_passphrase.data(), i_passphrase.size());
    m_ctxt.m_cipher.set_key(dig.data(), dig.size());

    // Create zero blocks for sparse file reads.
    m_ctxt.m_zdatobj = new ZeroDataBlockNode();
    m_ctxt.m_zsinobj = new ZeroIndirectBlockNode(m_ctxt.m_zdatobj);
    m_ctxt.m_zdinobj = new ZeroDoubleIndBlockNode(m_ctxt.m_zsinobj);

    m_rdh = new RootDirNode();
    
    rootref(m_rdh->bn_flush(m_ctxt));
}

void
UTFileSystem::fs_mount(BlockStoreHandle const & i_bsh,
                       string const & i_fsid,
                       string const & i_passphrase,
                       StringSeq const & i_args)
    throw (InternalError,
           ValueError,
           NotFoundError)
{
    LOG(lgr, 4, "fs_mount " << i_fsid);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    m_ctxt.m_bsh = i_bsh;

    // Save the digest of the fsid.
    m_fsiddig = Digest(i_fsid.data(), i_fsid.size());

    // Use part of the digest of the passphrase as the key.
    Digest dig(i_passphrase.data(), i_passphrase.size());
    m_ctxt.m_cipher.set_key(dig.data(), dig.size());

    // Create zero blocks for sparse file reads.
    m_ctxt.m_zdatobj = new ZeroDataBlockNode();
    m_ctxt.m_zsinobj = new ZeroIndirectBlockNode(m_ctxt.m_zdatobj);
    m_ctxt.m_zdinobj = new ZeroDoubleIndBlockNode(m_ctxt.m_zsinobj);

    try
    {
        m_rdh = new RootDirNode(m_ctxt, rootref());
    }
    catch (NotFoundError const & ex)
    {
        // Root directory not found.
        throwstream(NotFoundError, "filesystem root block not found");
    }
}

void
UTFileSystem::fs_umount()
    throw (InternalError)
{
    LOG(lgr, 4, "fs_umount ");

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    rootref(m_rdh->bn_flush(m_ctxt));

    m_rdh = NULL;
    m_ctxt.m_bsh = NULL;
    m_ctxt.m_cipher.unset_key();
    m_fsiddig = Digest();
}

class GetAttrTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    GetAttrTraverseFunc(struct stat * o_stbuf) : m_sbp(o_stbuf) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        nt_retval(i_fn.getattr(i_ctxt, m_sbp));
    }

private:
    struct stat *	m_sbp;
};

int
UTFileSystem::fs_getattr(string const & i_path,
                         struct stat * o_stbuf)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_getattr " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        GetAttrTraverseFunc gatf(o_stbuf);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_DEFAULT,
                             ps.first, ps.second, gatf);

        LOG(lgr, 6, "fs_getattr " << i_path << " -> " << gatf.nt_retval());
        return gatf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_getattr " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class ReadLinkTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    ReadLinkTraverseFunc(char * o_obuf, size_t i_size)
        : m_obuf(o_obuf), m_size(i_size) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        nt_retval(i_fn.readlink(i_ctxt, m_obuf, m_size));
    }

private:
    char *		m_obuf;
    size_t		m_size;
};

int
UTFileSystem::fs_readlink(string const & i_path,
                          char * o_obuf,
                          size_t i_size)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_readlink " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        ReadLinkTraverseFunc rltf(o_obuf, i_size);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_DEFAULT,
                             ps.first, ps.second, rltf);

        LOG(lgr, 6, "fs_readlink " << i_path << " -> " << rltf.nt_retval());
        return rltf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_readlink " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class MknodTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    MknodTraverseFunc(mode_t i_mode, dev_t i_dev)
        : m_mode(i_mode), m_dev(i_dev) {}

    virtual void nt_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        nt_retval(i_dn.mknod(i_ctxt, i_entry, m_mode, m_dev));
    }

private:
    mode_t		m_mode;
    dev_t		m_dev;
};

int
UTFileSystem::fs_mknod(string const & i_path,
                       mode_t i_mode,
                       dev_t i_dev)
        throw (InternalError)
{
    LOG(lgr, 6, "fs_mknod " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        MknodTraverseFunc otf(i_mode, i_dev);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT | DirNode::NT_UPDATE,
                             ps.first, ps.second, otf);

        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_mknod " << i_path << " -> " << otf.nt_retval());
        return otf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_mknod " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class MkdirTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    MkdirTraverseFunc(mode_t i_mode) : m_mode(i_mode) {}

    virtual void nt_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        nt_retval(i_dn.mkdir(i_ctxt, i_entry, m_mode));
    }

private:
    mode_t		m_mode;
};

int
UTFileSystem::fs_mkdir(string const & i_path, mode_t i_mode)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_mkdir " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        MkdirTraverseFunc otf(i_mode);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT | DirNode::NT_UPDATE,
                             ps.first, ps.second, otf);

        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_mkdir " << i_path << " -> " << otf.nt_retval());
        return otf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_mkdir " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class UnlinkTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    UnlinkTraverseFunc(bool i_dirstoo) : m_dirstoo(i_dirstoo) {}

    virtual void nt_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        nt_retval(i_dn.unlink(i_ctxt, i_entry, m_dirstoo));
    }

private:
    bool	m_dirstoo;
};

int
UTFileSystem::fs_unlink(string const & i_path)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_unlink " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        UnlinkTraverseFunc utf(false);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT | DirNode::NT_UPDATE,
                             ps.first, ps.second, utf);

        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_unlink " << i_path << " -> " << utf.nt_retval());
        return utf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_unlink " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class RmdirTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    RmdirTraverseFunc() {}

    virtual void nt_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        nt_retval(i_dn.rmdir(i_ctxt, i_entry));
    }

private:
};

int
UTFileSystem::fs_rmdir(string const & i_path)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_rmdir " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        RmdirTraverseFunc rtf;
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT | DirNode::NT_UPDATE,
                             ps.first, ps.second, rtf);

        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_rmdir " << i_path << " -> " << rtf.nt_retval());
        return rtf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_rmdir " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class SymlinkTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    SymlinkTraverseFunc(string const & i_opath) : m_opath(i_opath) {}

    virtual void nt_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        nt_retval(i_dn.symlink(i_ctxt, i_entry, m_opath));
    }

private:
    string		m_opath;
};

int
UTFileSystem::fs_symlink(string const & i_opath, string const & i_npath)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_symlink " << i_opath << ' ' << i_npath);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_npath);
        SymlinkTraverseFunc stf(i_opath);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT | DirNode::NT_UPDATE,
                             ps.first, ps.second, stf);

        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_symlink " << i_opath << ' ' << i_npath
            << " -> " << stf.nt_retval());
        return stf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_symlink " << i_opath << ' ' << i_npath
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class LinkSrcTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    LinkSrcTraverseFunc() {}

    virtual void nt_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        m_blkref = i_dn.linksrc(i_ctxt, i_entry);
    }

    BlockRef const & blkref() const { return m_blkref; }

private:
    BlockRef	m_blkref;
};

class LinkDstTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    LinkDstTraverseFunc(BlockRef const & i_blkref, bool i_force)
        : m_blkref(i_blkref)
        , m_force(i_force)
    {}

    virtual void nt_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        nt_retval(i_dn.linkdst(i_ctxt, i_entry, m_blkref, m_force));
    }

private:
    BlockRef	m_blkref;
    bool		m_force;
};

int
UTFileSystem::fs_rename(string const & i_opath, string const & i_npath)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_rename " << i_opath << ' ' << i_npath);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ops = DirNode::pathsplit(i_opath);
        pair<string, string> nps = DirNode::pathsplit(i_npath);

        // First determine the blkref of the source.
        LinkSrcTraverseFunc lstf;
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT,
                             ops.first, ops.second, lstf);

        // Then add the blkref as the new name.
        LinkDstTraverseFunc ldtf(lstf.blkref(), true);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT | DirNode::NT_UPDATE,
                             nps.first, nps.second, ldtf);

        // Then unlink the old path.
        UnlinkTraverseFunc utf(true);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT | DirNode::NT_UPDATE,
                             ops.first, ops.second, utf);

        // Update the root reference.
        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_rename " << i_opath << ' ' << i_npath
            << " -> " << utf.nt_retval());
        return utf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_rename " << i_opath << ' ' << i_npath
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

int
UTFileSystem::fs_link(string const & i_opath, string const & i_npath)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_link " << i_opath << ' ' << i_npath);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ops = DirNode::pathsplit(i_opath);
        pair<string, string> nps = DirNode::pathsplit(i_npath);

        // First determine the blkref of the source.
        LinkSrcTraverseFunc lstf;
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT,
                             ops.first, ops.second, lstf);

        // Then add the blkref as the new name.
        LinkDstTraverseFunc ldtf(lstf.blkref(), false);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT | DirNode::NT_UPDATE,
                             nps.first, nps.second, ldtf);

        // Update the root reference.
        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_link " << i_opath << ' ' << i_npath
            << " -> " << ldtf.nt_retval());
        return ldtf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_link " << i_opath << ' ' << i_npath
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class ChmodTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    ChmodTraverseFunc(mode_t i_mode) : m_mode(i_mode) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        nt_retval(i_fn.chmod(i_ctxt, m_mode));
    }

private:
    mode_t		m_mode;
};

int
UTFileSystem::fs_chmod(string const & i_path, mode_t i_mode)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_chmod " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        ChmodTraverseFunc ctf(i_mode);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_UPDATE,
                             ps.first, ps.second, ctf);

        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_chmod " << i_path << " -> " << ctf.nt_retval());
        return ctf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_chmod " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class TruncateTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    TruncateTraverseFunc(off_t i_size) : m_size(i_size) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        nt_retval(i_fn.truncate(i_ctxt, m_size));
    }

private:
    off_t		m_size;
};

int
UTFileSystem::fs_truncate(string const & i_path, off_t i_size)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_truncate " << i_path << ' ' << i_size);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        TruncateTraverseFunc ttf(i_size);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_UPDATE,
                             ps.first, ps.second, ttf);

        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_truncate " << i_path << " -> " << ttf.nt_retval());
        return ttf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_truncate " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class OpenTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    OpenTraverseFunc(int i_flags) : m_flags(i_flags) {}

    virtual void nt_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        nt_retval(i_dn.open(i_ctxt, i_entry, m_flags));
    }

private:
    int		m_flags;
};

int
UTFileSystem::fs_open(string const & i_path, int i_flags)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_open " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        OpenTraverseFunc otf(i_flags);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_PARENT,
                             ps.first, ps.second, otf);
        LOG(lgr, 6, "fs_open " << i_path << " -> " << otf.nt_retval());
        return otf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_open " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class ReadTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    ReadTraverseFunc(void * i_bufptr, size_t i_size, off_t i_off)
        : m_bufptr(i_bufptr), m_size(i_size), m_off(i_off) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        nt_retval(i_fn.read(i_ctxt, m_bufptr, m_size, m_off));
    }

private:
    void *			m_bufptr;
    size_t			m_size;
    off_t			m_off;
};

int
UTFileSystem::fs_read(string const & i_path,
                      void * o_bufptr,
                      size_t i_size,
                      off_t i_off)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_read " << i_path << " sz=" << i_size << " off=" << i_off);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        ReadTraverseFunc wtf(o_bufptr, i_size, i_off);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_DEFAULT,
                             ps.first, ps.second, wtf);
        LOG(lgr, 6, "fs_read " << i_path << " -> " << wtf.nt_retval());
        return wtf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_read " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class WriteTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    WriteTraverseFunc(void const * i_bufptr, size_t i_size, off_t i_off)
        : m_bufptr(i_bufptr), m_size(i_size), m_off(i_off) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        nt_retval(i_fn.write(i_ctxt, m_bufptr, m_size, m_off));
    }

private:
    void const *	m_bufptr;
    size_t			m_size;
    off_t			m_off;
};

int
UTFileSystem::fs_write(string const & i_path,
                       void const * i_data,
                       size_t i_size,
                       off_t i_off)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_write " << i_path << " sz=" << i_size << " off=" << i_off);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        WriteTraverseFunc wtf(i_data, i_size, i_off);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_UPDATE,
                             ps.first, ps.second, wtf);
        rootref(m_rdh->bn_blkref());
        LOG(lgr, 6, "fs_write " << i_path << " -> " << wtf.nt_retval());
        return wtf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_write " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class ReadDirTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    ReadDirTraverseFunc(off_t i_offset,
                        FileSystem::DirEntryFunc & i_entryfunc)
        : m_offset(i_offset), m_entryfunc(i_entryfunc) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        // Cast the node to a directory node.
        DirNodeHandle dh = dynamic_cast<DirNode *>(&i_fn);
        if (!dh)
            throw ENOTDIR;

        nt_retval(dh->readdir(i_ctxt, m_offset, m_entryfunc));
    }

private:
    off_t							m_offset;
    FileSystem::DirEntryFunc &		m_entryfunc;
};


int
UTFileSystem::fs_readdir(string const & i_path,
                         off_t i_offset,
                         DirEntryFunc & o_entryfunc)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_readdir " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        ReadDirTraverseFunc rdtf(i_offset, o_entryfunc);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_DEFAULT,
                             ps.first, ps.second, rdtf);
        LOG(lgr, 6, "fs_readdir " << i_path << " -> " << rdtf.nt_retval());
        return rdtf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_readdir " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class AccessTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    AccessTraverseFunc(int i_mode) : m_mode(i_mode) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        nt_retval(i_fn.access(i_ctxt, m_mode));
    }

private:
    int		m_mode;
};

int
UTFileSystem::fs_access(string const & i_path, int i_mode)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_access " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        AccessTraverseFunc atf(i_mode);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_DEFAULT,
                             ps.first, ps.second, atf);
        LOG(lgr, 6, "fs_access " << i_path << " -> " << atf.nt_retval());
        return atf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_access " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

class UtimeTraverseFunc : public DirNode::NodeTraverseFunc
{
public:
    UtimeTraverseFunc(T64 const & i_atime, T64 const & i_mtime)
        : m_atime(i_atime), m_mtime(i_mtime) {}

    virtual void nt_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        i_fn.atime(m_atime);
        i_fn.mtime(m_mtime);
        nt_retval(0);
    }

private:
    T64		m_atime;
    T64		m_mtime;
};

int
UTFileSystem::fs_utime(string const & i_path,
                       T64 const & i_atime,
                       T64 const & i_mtime)
    throw (InternalError)
{
    LOG(lgr, 6, "fs_utime " << i_path << ' ' << i_atime << ' ' << i_mtime);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        UtimeTraverseFunc otf(i_atime, i_mtime);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_UPDATE,
                             ps.first, ps.second, otf);

        // rootref(m_rdh->bn_flush(m_ctxt));

        LOG(lgr, 6, "fs_utime " << i_path << " -> " << otf.nt_retval());
        return otf.nt_retval();
    }
    catch (int const & i_errno)
    {
        LOG(lgr, 6, "fs_utime " << i_path
            << ": " << ACE_OS::strerror(i_errno));
        return -i_errno;
    }
}

size_t
UTFileSystem::fs_refresh()
    throw (InternalError)
{
    LOG(lgr, 6, "fs_refresh");

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    // Make sure all cached nodes are up-to-date in blockstore.
    rootref(m_rdh->bn_flush(m_ctxt));

    size_t nb = m_rdh->rb_refresh(m_ctxt);

    LOG(lgr, 6, "fs_refresh -> " << nb);

    return nb;
}

void
UTFileSystem::rootref(BlockRef const & i_blkref)
{
    LOG(lgr, 6, "rootref set " << i_blkref);

    uint8 iv[16];
    memset(iv, '\0', sizeof(iv));

    uint8 buffer[sizeof(BlockRef)];
    ACE_OS::memcpy(buffer, i_blkref.data(), sizeof(buffer));

    m_ctxt.m_cipher.encrypt(iv, buffer, sizeof(buffer));

    m_ctxt.m_bsh->bs_put_block(m_fsiddig.data(), m_fsiddig.size(),
                               buffer, sizeof(buffer));
}

BlockRef
UTFileSystem::rootref()
{
    string refstr;
    refstr.resize(sizeof(BlockRef));

    m_ctxt.m_bsh->bs_get_block(m_fsiddig.data(), m_fsiddig.size(),
                               &refstr[0], refstr.size());

    // Can this block be validated?

    uint8 iv[16];
    memset(iv, '\0', sizeof(iv));

    m_ctxt.m_cipher.decrypt(iv, (uint8 *) &refstr[0], refstr.size());

    BlockRef blkref(refstr);

    LOG(lgr, 6, "rootref get " << blkref);

    return blkref;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
