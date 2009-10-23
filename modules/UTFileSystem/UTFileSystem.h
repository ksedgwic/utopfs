#ifndef UTFS_UTFileSystem_h__
#define UTFS_UTFileSystem_h__

/// @file UTFileSystem.h
/// Utopia FileSystem Instance.

#include <map>
#include <string>

#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "BlockCipher.h"
#include "BlockRef.h"
#include "Digest.h"
#include "FileSystem.h"
#include "Types.h"

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

    virtual void fs_mkfs(utp::BlockStoreHandle const & i_bsh,
                         std::string const & i_fsid,
                         std::string const & i_passphrase,
                         std::string const & i_uname,
                         std::string const & i_gname,
                         utp::StringSeq const & i_args)
        throw (utp::InternalError,
               utp::ValueError);

    virtual void fs_mount(utp::BlockStoreHandle const & i_bsh,
                          std::string const & i_fsid,
                          std::string const & i_passphrase,
                          utp::StringSeq const & i_args)
        throw (utp::InternalError,
               utp::ValueError,
               utp::NotFoundError);

    virtual void fs_umount()
        throw (utp::InternalError,
               utp::NoSpaceError);

    virtual int fs_getattr(std::string const & i_path,
                           struct statstb * o_statbuf)
        throw (utp::InternalError);

    virtual int fs_readlink(std::string const & i_path,
                            char * o_obuf,
                            size_t i_size)
        throw (utp::InternalError);

    virtual int fs_mknod(std::string const & i_path,
                         mode_t i_mode,
                         dev_t i_dev,
                         std::string const & i_uname,
                         std::string const & i_gname)
        throw (utp::InternalError);

    virtual int fs_mkdir(std::string const & i_path,
                         mode_t i_mode,
                         std::string const & i_uname,
                         std::string const & i_gname)
        throw (utp::InternalError);

    virtual int fs_unlink(std::string const & i_path)
        throw (utp::InternalError);

    virtual int fs_rmdir(std::string const & i_path)
        throw (utp::InternalError);

    virtual int fs_symlink(std::string const & i_opath,
                           std::string const & i_npath)
        throw (utp::InternalError);

    virtual int fs_rename(std::string const & i_opath,
                          std::string const & i_npath)
        throw (utp::InternalError);

    virtual int fs_link(std::string const & i_opath,
                        std::string const & i_npath)
        throw (utp::InternalError);

    virtual int fs_chmod(std::string const & i_path,
                         mode_t i_mode)
        throw (utp::InternalError);

    virtual int fs_truncate(std::string const & i_path,
                            off_t i_size)
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

    virtual int fs_statfs(struct ::statvfs * o_stvbuf)
        throw (utp::InternalError);

    virtual int fs_readdir(std::string const & i_path,
                           off_t i_offset,
                           DirEntryFunc & o_entryfunc)
        throw (utp::InternalError);

    virtual int fs_access(std::string const & i_path,
                          int i_mode)
        throw (utp::InternalError);

    virtual int fs_utime(std::string const & i_path,
                         utp::T64 const & i_atime,
                         utp::T64 const & i_mtime)
        throw (utp::InternalError);

    virtual size_t fs_refresh()
        throw (utp::InternalError);

    virtual void fs_sync()
        throw (utp::InternalError,
               utp::NoSpaceError);

    virtual void fs_get_stats(utp::StatSet & o_ss) const
        throw(utp::InternalError);

protected:
    void rootref(BlockRef const & i_blkref);

    BlockRef rootref();

private:
    utp::Digest					m_fsiddig;

    ACE_Thread_Mutex			m_utfsmutex;

    Context						m_ctxt;

    DirNodeHandle				m_rdh;

    utp::HeadNode				m_hn;

    BlockRef					m_rbr;

    utp::AtomicLong				m_nrdops;
    utp::AtomicLong				m_nwrops;
    utp::AtomicLong				m_nrdbytes;
    utp::AtomicLong				m_nwrbytes;
};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_UTFileSystem_h__
