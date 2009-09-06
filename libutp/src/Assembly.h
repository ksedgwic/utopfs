#ifndef utp_Assembly_h__
#define utp_Assembly_h__

/// @file Assembly.h
/// Utopia FileSystem Assembly Object.
///
/// Assembly Object for Utopfs FileSystem.

#include <iosfwd>

#include "Assembly.pb.h"

#include "utpfwd.h"
#include "utpexp.h"

namespace utp {

class UTP_EXP Assembly
{
public:
    Assembly(std::istream & i_cfgstrm);

    virtual ~Assembly();

    BlockStoreHandle const & bsh() const { return m_bsh; }

    FileSystemHandle const & fsh() const { return m_fsh; }

    void export_config(std::ostream & i_ostrm) const;

private:
    AssemblyConfig		m_asscfg;

    BlockStoreHandle	m_bsh;	// Root blockstore.
    FileSystemHandle	m_fsh;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Assembly_h__
