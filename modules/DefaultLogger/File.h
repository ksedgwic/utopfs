#ifndef DFLG_File_h__
#define DFLG_File_h__

/// @file File.h
/// Defines the File Logger.

#include <string>
#include <fstream>

#include "Log.h"

#include "Stream.h"
#include "dflgexp.h"

namespace DFLG {

/// A specialized Stream Logger which uses a File.
class DFLG_EXP File : public Stream
{
public:
    /// Constructor.
    /// @param level initial logging level.
    /// @param path log file path.
    /// @param flushflag should flush after each write?
    File(int i_level, std::string const & i_path, bool i_flushflag = true)
        : Stream(i_level, static_cast<std::ostream *>
                 (new std::ofstream(i_path.c_str(), ios::app)),
                 i_flushflag)
    {}

    /// Destructor.
    virtual ~File() {}
};

} // end namespace DFLG

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // DFLG_File_h__
