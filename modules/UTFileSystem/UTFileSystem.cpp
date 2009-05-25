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
                      string const & i_passphrase)
    throw (utp::InternalError,
           utp::ValueError)
{
    LOG(lgr, 4, "fs_mkfs " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    m_ctxt.m_bsh = BlockStore::instance();
    m_ctxt.m_bsh->bs_create(i_path);

    // Use part of the digest of the passphrase as the key.
    Digest dig(i_passphrase.data(), i_passphrase.size());
    m_ctxt.m_cipher.set_key(dig.data(), dig.size());

    m_rdh = new RootDirNode();
    m_rdh->persist(m_ctxt);
    rootref(m_rdh->digest());
}

void
UTFileSystem::fs_mount(string const & i_path,
                       string const & i_passphrase)
    throw (utp::InternalError,
           utp::ValueError)
{
    LOG(lgr, 4, "fs_mount " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    m_ctxt.m_bsh = BlockStore::instance();
    m_ctxt.m_bsh->bs_open(i_path);

    // Use part of the digest of the passphrase as the key.
    Digest dig(i_passphrase.data(), i_passphrase.size());
    m_ctxt.m_cipher.set_key(dig.data(), dig.size());

    m_rdh = new RootDirNode(m_ctxt, rootref());
}

void
UTFileSystem::fs_unmount()
    throw (utp::InternalError)
{
    LOG(lgr, 4, "fs_unmount ");

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    m_rdh = NULL;
    m_ctxt.m_bsh = NULL;
    m_ctxt.m_cipher.unset_key();
}

class GetAttrTraverseFunc : public TraverseFunc
{
public:
    GetAttrTraverseFunc(struct stat * o_stbuf) : m_sbp(o_stbuf) {}

    virtual void tf_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        retval(i_fn.getattr(i_ctxt, m_sbp));
    }

private:
    struct stat *	m_sbp;
};

int
UTFileSystem::fs_getattr(string const & i_path,
                         struct stat * o_stbuf)
    throw (utp::InternalError)
{
    LOG(lgr, 6, "fs_getattr " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        GetAttrTraverseFunc gatf(o_stbuf);
        m_rdh->traverse(m_ctxt, DirNode::TF_DEFAULT,
                        ps.first, ps.second, gatf);
        return gatf.retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

class OpenTraverseFunc : public TraverseFunc
{
public:
    OpenTraverseFunc(int i_flags) : m_flags(i_flags) {}

    virtual void tf_parent(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry)
    {
        retval(i_dn.open(i_ctxt, i_entry, m_flags));
    }

    virtual void tf_update(Context & i_ctxt,
                           DirNode & i_dn,
                           string const & i_entry,
                           Digest const & i_dig)
    {
        i_dn.update(i_ctxt, i_entry, i_dig);
    }

private:
    int		m_flags;
};

int
UTFileSystem::fs_open(string const & i_path, int i_flags)
    throw (utp::InternalError)
{
    LOG(lgr, 6, "fs_open " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        OpenTraverseFunc otf(i_flags);
        m_rdh->traverse(m_ctxt, DirNode::TF_PARENT | DirNode::TF_UPDATE,
                        ps.first, ps.second, otf);
        rootref(m_rdh->digest());
        return otf.retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

class ReadTraverseFunc : public TraverseFunc
{
public:
    ReadTraverseFunc(void * i_bufptr, size_t i_size, off_t i_off)
        : m_bufptr(i_bufptr), m_size(i_size), m_off(i_off) {}

    virtual void tf_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        retval(i_fn.read(i_ctxt, m_bufptr, m_size, m_off));
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
    throw (utp::InternalError)
{
    LOG(lgr, 6, "fs_read " << i_path << " sz=" << i_size << " off=" << i_off);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        ReadTraverseFunc wtf(o_bufptr, i_size, i_off);
        m_rdh->traverse(m_ctxt, DirNode::TF_DEFAULT,
                        ps.first, ps.second, wtf);
        return wtf.retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

class WriteTraverseFunc : public TraverseFunc
{
public:
    WriteTraverseFunc(void const * i_bufptr, size_t i_size, off_t i_off)
        : m_bufptr(i_bufptr), m_size(i_size), m_off(i_off) {}

    virtual void tf_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        retval(i_fn.write(i_ctxt, m_bufptr, m_size, m_off));
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
    throw (utp::InternalError)
{
    LOG(lgr, 6, "fs_write " << i_path << " sz=" << i_size << " off=" << i_off);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        WriteTraverseFunc wtf(i_data, i_size, i_off);
        m_rdh->traverse(m_ctxt, DirNode::TF_UPDATE,
                        ps.first, ps.second, wtf);
        return wtf.retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

class ReadDirTraverseFunc : public TraverseFunc
{
public:
    ReadDirTraverseFunc(off_t i_offset,
                        FileSystem::DirEntryFunc & i_entryfunc)
        : m_offset(i_offset), m_entryfunc(i_entryfunc) {}

    virtual void tf_leaf(Context & i_ctxt, FileNode & i_fn)
    {
        // Cast the node to a directory node.
        DirNodeHandle dh = dynamic_cast<DirNode *>(&i_fn);
        if (!dh)
            throw ENOTDIR;

        retval(dh->readdir(i_ctxt, m_offset, m_entryfunc));
    }

private:
    off_t							m_offset;
    FileSystem::DirEntryFunc &		m_entryfunc;
};


int
UTFileSystem::fs_readdir(string const & i_path,
                         off_t i_offset,
                         DirEntryFunc & o_entryfunc)
    throw (utp::InternalError)
{
    LOG(lgr, 6, "fs_readdir " << i_path);

    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        ReadDirTraverseFunc rdtf(i_offset, o_entryfunc);
        m_rdh->traverse(m_ctxt, DirNode::TF_DEFAULT,
                        ps.first, ps.second, rdtf);
        return rdtf.retval();
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

int
UTFileSystem::fs_create(string const & i_path,
                        mode_t i_mode)
        throw (utp::InternalError)
{
    LOG(lgr, 6, "fs_create " << i_path);

    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_create unimplemented");
}

void
UTFileSystem::rootref(utp::Digest const & i_digest)
{
    LOG(lgr, 6, "rootref set " << i_digest);

    uint8 iv[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    uint8 buffer[sizeof(Digest)];
    ACE_OS::memcpy(buffer, i_digest.data(), sizeof(buffer));

    m_ctxt.m_cipher.encrypt(iv, 0, buffer, sizeof(buffer));

    string key = "ROOT";
    
    m_ctxt.m_bsh->bs_put_block(key.data(), key.size(),
                               buffer, sizeof(buffer));
}

Digest
UTFileSystem::rootref()
{
    string digstr;
    digstr.resize(sizeof(Digest));

    string key = "ROOT";
    
    m_ctxt.m_bsh->bs_get_block(key.data(), key.size(),
                               &digstr[0], digstr.size());
    
    uint8 iv[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    m_ctxt.m_cipher.encrypt(iv, 0, (uint8 *) &digstr[0], digstr.size());

    Digest dig(digstr);

    LOG(lgr, 6, "rootref get " << dig);

    return dig;
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End: