#include <sstream>
#include <string>

#include <ace/Guard_T.h>

#include "Base32.h"
#include "BlockStore.h"
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
UTFileSystem::fs_mkfs(string const & i_path,
                      string const & i_fsid,
                      string const & i_passphrase)
    throw (InternalError,
           ValueError)
{
    LOG(lgr, 4, "fs_mkfs " << i_fsid << ' ' << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    m_ctxt.m_bsh = BlockStore::instance();
    m_ctxt.m_bsh->bs_create(i_path);

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
    m_rdh->persist(m_ctxt);
    rootref(m_rdh->bn_blkref());
}

void
UTFileSystem::fs_mount(string const & i_path,
                       string const & i_fsid,
                       string const & i_passphrase)
    throw (InternalError,
           ValueError,
           NotFoundError)
{
    LOG(lgr, 4, "fs_mount " << i_fsid << ' ' << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    m_ctxt.m_bsh = BlockStore::instance();
    m_ctxt.m_bsh->bs_open(i_path);

    // Save the digest of the fsid.
    m_fsiddig = Digest(i_fsid.data(), i_fsid.size());

    // Use part of the digest of the passphrase as the key.
    Digest dig(i_passphrase.data(), i_passphrase.size());
    m_ctxt.m_cipher.set_key(dig.data(), dig.size());

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
UTFileSystem::fs_unmount()
    throw (InternalError)
{
    LOG(lgr, 4, "fs_unmount ");

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

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
        return gatf.nt_retval();
    }
    catch (int const & i_errno)
    {
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
        rootref(m_rdh->bn_blkref());
        return otf.nt_retval();
    }
    catch (int const & i_errno)
    {
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
        rootref(m_rdh->bn_blkref());
        return otf.nt_retval();
    }
    catch (int const & i_errno)
    {
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
        rootref(m_rdh->bn_blkref());
        return ctf.nt_retval();
    }
    catch (int const & i_errno)
    {
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
        // rootref(m_rdh->bn_blkref());
        return otf.nt_retval();
    }
    catch (int const & i_errno)
    {
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
        return wtf.nt_retval();
    }
    catch (int const & i_errno)
    {
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
        return wtf.nt_retval();
    }
    catch (int const & i_errno)
    {
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
        return rdtf.nt_retval();
    }
    catch (int const & i_errno)
    {
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
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        UtimeTraverseFunc otf(i_atime, i_mtime);
        m_rdh->node_traverse(m_ctxt, DirNode::NT_UPDATE,
                             ps.first, ps.second, otf);
        rootref(m_rdh->bn_blkref());
        return otf.nt_retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
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
