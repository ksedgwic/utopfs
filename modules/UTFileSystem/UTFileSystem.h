#ifndef UTFS_UTFileSystem_h__
#define UTFS_UTFileSystem_h__

/// @file UTFileSystem.h
/// Utopia FileSystem Instance.

#include <map>
#include <string>

#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "BlockCipher.h"
#include "Digest.h"
#include "FileSystem.h"

#include "Context.h"

#include "utfsexp.h"
#include "utfsfwd.h"

namespace UTFS {

class UTFS_EXP UTFileSystem : public utp::FileSystem
{
public:
    UTFileSystem();

    virtual ~UTFileSystem();

    // FileSystem methods.

    virtual void fs_mkfs(std::string const & i_path,
                         std::string const & i_fsid,
                         std::string const & i_passphrase)
        throw (utp::InternalError,
               utp::ValueError);

    virtual void fs_mount(std::string const & i_path,
                          std::string const & i_fsid,
                          std::string const & i_passphrase)
        throw (utp::InternalError,
               utp::ValueError,
               utp::NotFoundError);

    virtual void fs_unmount()
        throw (utp::InternalError);

    virtual int fs_getattr(std::string const & i_path,
                           struct stat * o_statbuf)
        throw (utp::InternalError);

    virtual int fs_mknod(std::string const & i_path,
                         mode_t i_mode,
                         dev_t i_dev)
        throw (utp::InternalError);

    virtual int fs_mkdir(std::string const & i_path,
                         mode_t i_mode)
        throw (utp::InternalError);

    virtual int fs_unlink(std::string const & i_path)
        throw (utp::InternalError);

    virtual int fs_chmod(std::string const & i_path,
                         mode_t i_mode)
        throw (utp::InternalError);

    virtual int fs_open(std::string const & i_path,
                        int i_flags)
        throw (utp::InternalError);

    virtual int fs_read(std::string const & i_path,
                        void * o_bufptr,
                        size_t i_size,
                        off_t i_off)
        throw (utp::InternalError);

    virtual int fs_write(std::string const & i_path,
                         void const * i_data,
                         size_t i_size,
                         off_t i_off)
        throw (utp::InternalError);

    virtual int fs_readdir(std::string const & i_path,
                           off_t i_offset,
                           DirEntryFunc & o_entryfunc)
        throw (utp::InternalError);

    virtual int fs_utime(std::string const & i_path,
                         utp::T64 const & i_atime,
                         utp::T64 const & i_mtime)
        throw (utp::InternalError);

protected:
    void rootref(BlockRef const & i_blkref);

    BlockRef rootref();

private:
    typedef std::map<std::string, FileNodeHandle>	NodeMap;

    utp::Digest				m_fsiddig;

    ACE_Thread_Mutex		m_utfsmutex;

    Context					m_ctxt;

    DirNodeHandle			m_rdh;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_UTFileSystem_h__
