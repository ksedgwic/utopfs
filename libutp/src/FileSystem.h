#ifndef utp_FileSystem_h__
#define utp_FileSystem_h__

/// @file FileSystem.h
/// Abstract FileSystem Interface.

#include <string>

#include "utpexp.h"
#include "utpfwd.h"

#include "Except.h"
#include "RC.h"

namespace utp {

class UTP_EXP FileSystem : public virtual RCObj
{
public:
    struct DirEntryFunc
    {
        virtual bool operator()(std::string const & i_name, off_t i_off) = 0;
    };

    /// Assign the singleton instance
    ///
    static void instance(FileSystemHandle const & i_ash);

    /// Retrieve the singleton instance
    ///
    static FileSystemHandle instance();

    /// Destructor
    ///
    virtual ~FileSystem();

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
                           struct stat * o_statbuf)
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
    /// @param[in] i_off Number of bytes to skip before reading.
    ///
    /// @return Returns 0 on success or errno value otherwise.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    ///
    virtual int fs_read(std::string const & i_path,
                        void * o_bufptr,
                        size_t i_size,
                        off_t i_off)
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

    
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_FileSystem_h__
