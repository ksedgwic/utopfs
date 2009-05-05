#ifndef DFLG_Console_h__
#define DFLG_Console_h__

/// @file Console.h
/// Defines the Console Logger.

#include <string>
#include <iostream>

#include "Log.h"

#include "Stream.h"
#include "dflgexp.h"

namespace DFLG {

/// A specialized Stream which outputs to the console.
class DFLG_EXP Console : public Stream
{
public:
    /// Constructor.
    Console(int i_level)
        : Stream(i_level, std::cerr, false)
    {
    }

    /// Destructor.
    virtual ~Console() {}
};

} // end namespace DFLG

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // DFLG_Console_h__
