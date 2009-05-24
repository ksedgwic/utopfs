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

int
UTFileSystem::fs_getattr(string const & i_path,
                         struct stat * o_stbuf)
    throw (utp::InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        FileNodeHandle nh = m_rdh->resolve(i_path);
        return nh->getattr(o_stbuf);
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

int
UTFileSystem::fs_open(string const & i_path,
                      int i_flags)
    throw (utp::InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        pair<DirNodeHandle, string> res = m_rdh->resolve_parent(i_path);
        return res.first->open(res.second, i_flags);
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

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
        FileNodeHandle nh = m_rdh->resolve(i_path);
        return nh->read(o_bufptr, i_size, i_off);
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

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
        FileNodeHandle nh = m_rdh->resolve(i_path);
        int rv = nh->write(i_data, i_size, i_off);
        nh->persist(m_bsh, m_cipher);
        return rv;
    }
    catch (int const & i_errno)
    {
        return -i_errno;
    }
}

int
UTFileSystem::fs_readdir(string const & i_path,
                         off_t i_offset,
                         DirEntryFunc & o_entryfunc)
    throw (utp::InternalError)
{
    ACE_Guard<ACE_Thread_Mutex> guard(m_utfsmutex);

    try
    {
        // Resolve the directory node.
        FileNodeHandle nh = m_rdh->resolve(i_path);

        // Cast the node to a directory node.
        DirNodeHandle dh = dynamic_cast<DirNode *>(&*nh);
        if (!dh)
            throw ENOTDIR;

        return dh->readdir(i_offset, o_entryfunc);
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
