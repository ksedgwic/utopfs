#ifndef UTFileSystem_h__
#define UTFileSystem_h__

/// @file UTFileSystem.h
/// Utopia FileSystem Instance.

#include <string>

#include "utpfwd.h"

#include "FileSystem.h"

#include "utfsexp.h"


namespace UTFS {

class UTFS_EXP UTFileSystem : public utp::FileSystem
{
public:
    UTFileSystem();

    virtual ~UTFileSystem();

    // FileSystem methods.

    virtual int fs_getattr(std::string const & i_path,
                           struct stat * o_statbuf)
        throw (utp::InternalError);

    virtual int fs_open(std::string const & i_path,
                        int i_flags)
        throw (utp::InternalError);

    virtual int fs_read(std::string const & i_path,
                        void * o_bufptr,
                        size_t i_size,
                        off_t i_off)
        throw (utp::InternalError);

    virtual int fs_readdir(std::string const & i_path,
                           off_t i_offset,
                           DirEntryFunc & o_entryfunc)
        throw (utp::InternalError);

private:

};

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFileSystem_h__
