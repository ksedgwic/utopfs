#ifndef UTFS_BlockRef_h__
#define UTFS_BlockRef_h__

/// @file BlockRef.h
/// Utopia FileSystem Block Reference.

#include <string>

#include "Types.h"

#include "utpfwd.h"
#include "utfsexp.h"

namespace UTFS {

class UTFS_EXP BlockRef
{
public:
    // Generate a nil reference.
    BlockRef();

    // Construct a reference from a digest.
    BlockRef(utp::Digest const & i_dig);

    /// Cast reference to a protobuf string value.
    ///
    /// IMPORTANT - This is marshals the BlockRef into a binary
    /// value held in a string.  This value is NOT printable!
    ///
    operator std::string() const
    {
        return std::string((char const *) m_ref, sizeof(m_ref));
    }

    /// Nil predicate.
    bool operator!() const;

    /// Non-nil predicate.
    operator bool() const { return !operator!(); }

    utp::uint8 const * data() const { return m_ref; }

    size_t size() const { return sizeof(m_ref); }

    // Return a pointer to the initvector portion of the reference.
    utp::uint8 const * iv() const { return &m_ref[16]; }

    // Validate a data block.  Returns true of the block matches
    // the reference, false otherwise.
    //
    bool validate(utp::uint8 const * i_data, size_t i_size) const;

private:
    utp::uint8				m_ref[32];
};

// Insert human friendly vesion of the reference into stream.
std::ostream & operator<<(std::ostream & strm, BlockRef const & i_ref);

} // namespace UTFS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // UTFS_BlockRef_h__
