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
    m_bsh = BlockStore::instance();
    m_bsh->bs_create(i_path);

    // Use part of the digest of the passphrase as the key.
    Digest dig(i_passphrase.data(), i_passphrase.size());
    m_cipher.set_key(dig.data(), dig.size());

    m_rdh = new RootDirNode();

    m_rdh->persist(m_bsh, m_cipher);
}

void
UTFileSystem::fs_mount(string const & i_path,
                       string const & i_passphrase)
    throw (utp::InternalError,
           utp::ValueError)
{
    m_bsh = BlockStore::instance();
    m_bsh->bs_open(i_path);

    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_mount unimplemented");

    // Use part of the digest of the passphrase as the key.
    Digest dig(i_passphrase.data(), i_passphrase.size());
    m_cipher.set_key(dig.data(), dig.size());

    m_rdh = new RootDirNode(/* digest of root block */);
}

class GetAttrTraverseFunc : public TraverseFunc
{
public:
    GetAttrTraverseFunc(struct stat * o_stbuf) : m_sbp(o_stbuf) {}

    virtual void tf_leaf(FileNode & i_fn)
    {
        retval(i_fn.getattr(m_sbp));
    }

private:
    struct stat *	m_sbp;
};

int
UTFileSystem::fs_getattr(string const & i_path,
                         struct stat * o_stbuf)
    throw (utp::InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        GetAttrTraverseFunc gatf(o_stbuf);
        m_rdh->traverse(m_bsh, m_cipher, ps.first, ps.second, gatf);
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

    virtual void tf_parent(DirNode & i_dn, string const & i_entry)
    {
        retval(i_dn.open(i_entry, m_flags));
    }

private:
    int		m_flags;
};

int
UTFileSystem::fs_open(string const & i_path, int i_flags)
    throw (utp::InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        OpenTraverseFunc otf(i_flags);
        m_rdh->traverse(m_bsh, m_cipher, ps.first, ps.second, otf);
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

    virtual void tf_leaf(FileNode & i_fn)
    {
        retval(i_fn.read(m_bufptr, m_size, m_off));
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
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        ReadTraverseFunc wtf(o_bufptr, i_size, i_off);
        m_rdh->traverse(m_bsh, m_cipher, ps.first, ps.second, wtf);
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

    virtual void tf_leaf(FileNode & i_fn)
    {
        retval(i_fn.write(m_bufptr, m_size, m_off));
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
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        WriteTraverseFunc wtf(i_data, i_size, i_off);
        m_rdh->traverse(m_bsh, m_cipher, ps.first, ps.second, wtf);
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
    ReadDirTraverseFunc(off_t i_offset, FileSystem::DirEntryFunc & i_entryfunc)
        : m_offset(i_offset), m_entryfunc(i_entryfunc) {}

    virtual void tf_leaf(FileNode & i_fn)
    {
        // Cast the node to a directory node.
        DirNodeHandle dh = dynamic_cast<DirNode *>(&i_fn);
        if (!dh)
            throw ENOTDIR;

        retval(dh->readdir(m_offset, m_entryfunc));
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
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<string, string> ps = DirNode::pathsplit(i_path);
        ReadDirTraverseFunc rdtf(i_offset, o_entryfunc);
        m_rdh->traverse(m_bsh, m_cipher, ps.first, ps.second, rdtf);
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
    throwstream(InternalError, FILELINE
                << "UTFileSystem::fs_create unimplemented");
}

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
