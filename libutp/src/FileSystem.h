#ifndef utp_FileSystem_h__
#define utp_FileSystem_h__

/// @file FileSystem.h
/// Abstract FileSystem Interface.

#if defined(WIN32)

#define FSTYPSZ     16
#define FSSTRSZ     32 //is it the volume name limit on windows?

struct statstb {
        _dev_t st_dev;
        _ino_t st_ino;
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        _dev_t st_rdev;
        _off_t st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
        long   st_blksize;
        long   st_blocks;
};

struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	unsigned long f_blocks;
    unsigned long f_bfree;
	unsigned long f_bavail;
	unsigned long f_files;
	unsigned long f_ffree;
	unsigned long f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
	unsigned long f_type;
	char f_basetype[FSTYPSZ];
	char f_str[FSSTRSZ];

}; 

#ifndef S_ISDIR
#define S_ISDIR(stmode)  \
    ((stmode & S_IFMT ) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(stmode)  \
    ((stmode & S_IFMT ) == S_IFREG)
#endif

#ifndef ALLPERMS
#define ALLPERMS (_O_WRONLY | _O_CREAT | _O_APPEND | _O_RDWR | \
                  _O_RDONLY | _O_TRUNC | _O_BINARY | _O_TEXT | \
                  _O_SEQUENTIAL | _O_RANDOM |_O_WTEXT )
#endif

#else
#include <sys/statvfs.h>
#define statstb stat
#endif

#include <string>

#include "utpexp.h"
#include "utpfwd.h"

#include "Stats.pb.h"

#include "Except.h"
#include "RC.h"
#include "T64.h"


namespace utp {

class UTP_EXP FileSystem : public virtual RCObj
{
public:
    struct DirEntryFunc
    {
        // @return true if buffer is full, false otherwise.
        //
        virtual bool def_entry(std::string const & i_name,
                               struct statstb const * i_stbuf,
                               off_t i_off) = 0;
    };

    /// Destructor
    ///
    virtual ~FileSystem();

    /// Create a filesystem
    ///
    /// @param[in] i_bsh BlockStore to hold filesystem data.
    /// @param[in] i_fsid FileSystem identifier.
    /// @param[in] i_passphrase FileSystem passphrase.
    /// @param[in] i_uname User name.
    /// @param[in] i_gname Group name.
    /// @param[in] i_args FileSystem specific arguments.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw ValueError Something is wrong with the path argument.
    ///
    virtual void fs_mkfs(BlockStoreHandle const & i_bsh,
                         std::string const & i_fsid,
                         std::string const & i_passphrase,
                         std::string const & i_uname,
                         std::string const & i_gname,
                         utp::StringSeq const & i_args)
        throw (utp::InternalError,
               utp::ValueError) = 0;

    /// Mount an existing filesystem
    ///
    /// @param[in] i_bsh BlockStore holding filesystem data.
    /// @param[in] i_fsid FileSystem identifier.
    /// @param[in] i_passphrase FileSystem passphrase.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw ValueError Something is wrong with the path argument.
    /// @throw NotFoundError Specified filesystem was not found.
    ///
    virtual void fs_mount(BlockStoreHandle const & i_bsh,
                          std::string const & i_fsid,
                          std::string const & i_passphrase,
                          utp::StringSeq const & i_args)
        throw (utp::InternalError,
               utp::ValueError,
               utp::NotFoundError) = 0;

    /// Unmount the filesystem
    ///
    /// After unmounting, you may create or mount again.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NoSpaceError There is not enough space.
    ///
    virtual void fs_umount()
        throw (utp::InternalError,
               utp::NoSpaceError) = 0;

    /// Get file attributes
    ///
    /// @param[in] i_path Path to the file.
    /// @param[out] o_statbuf Pointer to output stat structure.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_getattr(std::string const & i_path,
                           struct statstb * o_statbuf)
        throw (utp::InternalError) = 0;

    /// Read the target of a symbolic link.
    ///
    /// @param[in] i_path Path to the file.
    /// @param[out] o_obuf Pointer to output buffer.
    /// @param[in] i_size Size of output buffer.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_readlink(std::string const & i_path,
                            char * o_obuf,
                            size_t i_size)
        throw (utp::InternalError) = 0;

    /// Create a file node.
    ///
    /// @param[in] i_path Path to the file.
    /// @param[in] i_mode Permissions to use.
    /// @param[in] i_dev Some number.
    /// @param[in] i_uname User name.
    /// @param[in] i_gname Group name.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_mknod(std::string const & i_path,
                         mode_t i_mode,
                         dev_t i_dev,
                         std::string const & i_uname,
                         std::string const & i_gname)
        throw (utp::InternalError) = 0;

    /// Create a directory.
    ///
    /// @param[in] i_path Path to the directory.
    /// @param[in] i_mode File access flags.
    /// @param[in] i_uname User name.
    /// @param[in] i_gname Group name.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_mkdir(std::string const & i_path,
                         mode_t i_mode,
                         std::string const & i_uname,
                         std::string const & i_gname)
        throw (utp::InternalError) = 0;

    /// Remove a file.
    ///
    /// @param[in] i_path Path to the file.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_unlink(std::string const & i_path)
        throw (utp::InternalError) = 0;

    /// Remove a directory.
    ///
    /// @param[in] i_path Path to the directory.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_rmdir(std::string const & i_path)
        throw (utp::InternalError) = 0;

    /// Create a symbolic link.
    ///
    /// @param[in] i_opath Old path.
    /// @param[in] i_npath New path.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_symlink(std::string const & i_opath,
                           std::string const & i_npath)
        throw (utp::InternalError) = 0;

    /// Rename a file or directory.
    ///
    /// @param[in] i_opath Old path.
    /// @param[in] i_npath New path.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_rename(std::string const & i_opath,
                          std::string const & i_npath)
        throw (utp::InternalError) = 0;

    /// Create a hard link to a file or directory.
    ///
    /// @param[in] i_opath Old path.
    /// @param[in] i_npath New path.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_link(std::string const & i_opath,
                        std::string const & i_npath)
        throw (utp::InternalError) = 0;

    /// Change the permissions of a file or directory.
    ///
    /// @param[in] i_path Path to the file or directory.
    /// @param[in] i_mode Permission bits.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_chmod(std::string const & i_path,
                         mode_t i_mode)
        throw (utp::InternalError) = 0;

    /// Change the ownership of a file or directory.
    ///
    /// @param[in] i_path Path to the file or directory.
    /// @param[in] i_uname User name.
    /// @param[in] i_gname Group name.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_chown(std::string const & i_path,
                         std::string const & i_uname,
                         std::string const & i_gname)
        throw (utp::InternalError) = 0;

    /// Truncate a file to a specific length.
    ///
    /// @param[in] i_path Path to the directory.
    /// @param[in] i_size Size to set the file to.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_truncate(std::string const & i_path,
                            off_t i_size)
        throw (utp::InternalError) = 0;

    /// File open operation
    ///
    /// @param[in] i_path Path to the file.
    /// @param[in] i_flags File access flags.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_open(std::string const & i_path,
                        int i_flags)
        throw (utp::InternalError) = 0;

    /// Read data from an open file
    ///
    /// @param[in] i_path Path to the file.
    /// @param[out] o_bufptr Pointer to buffer for returned data.
    /// @param[in] i_size Number of bytes to read.
    /// @param[in] i_off Offset in file to read from.
    ///
    /// @return Returns bytes read on success or negative errno.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_read(std::string const & i_path,
                        void * o_bufptr,
                        size_t i_size,
                        off_t i_off)
        throw (utp::InternalError) = 0;

    /// Write data to an open file
    ///
    /// @param[in] i_path Path to the file.
    /// @param[in] i_data Pointer to data to be written.
    /// @param[in] i_size Number of bytes to written.
    /// @param[in] i_off Offset in file to write to.
    ///
    /// @return Returns bytes written on success or negative errno.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_write(std::string const & i_path,
                         void const * i_data,
                         size_t i_size,
                         off_t i_off)
        throw (utp::InternalError) = 0;

    /// Get filesystem statistics.
    ///
    /// @param[out] o_stvbuf Pointer to output structure.
    ///
    /// @return On success zero is returned or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_statfs(struct ::statvfs * o_stvbuf)
        throw (utp::InternalError) = 0;

    /// Read directory
    ///
    /// @param[in] i_path Path to the directory.
    /// @param[in] i_offset Number of entries to skip.
    /// @param[out] o_entryfunc DirEntryFunc to call on each entry.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_readdir(std::string const & i_path,
                           off_t i_offset,
                           DirEntryFunc & o_entryfunc)
        throw (utp::InternalError) = 0;

    /// Check file access permissions.
    ///
    /// @param[in] i_path Path to the file.
    /// @param[in] i_mode accessibility checks to be performed.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_access(std::string const & i_path,
                          int i_mode)
        throw (utp::InternalError) = 0;

    /// Change the access and modification times of a file.
    ///
    /// @param[in] i_path Path to the file.
    /// @param[in] i_tv Array of timestamps (access and mod).
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_utime(std::string const & i_path,
                         utp::T64 const & i_atime,
                         utp::T64 const & i_mtime)
        throw (utp::InternalError) = 0;

    /// Recursively refresh the filesystem in the blockstore.
    ///
    /// @return Returns the number of blocks refreshed.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual size_t fs_refresh()
        throw (utp::InternalError) = 0;

    /// Flush cached filesystem data to the blockstore.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NoSpaceError There is not enough space.
    ///
    virtual void fs_sync()
        throw (utp::InternalError,
               utp::NoSpaceError) = 0;

    /// Fetch statistics
    ///
    virtual void fs_get_stats(StatSet & o_ss) const
        throw(InternalError) = 0;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_FileSystem_h__
