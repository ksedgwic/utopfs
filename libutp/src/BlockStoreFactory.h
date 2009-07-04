#ifndef utp_BlockStoreFactory_h__
#define utp_BlockStoreFactory_h__

/// @file BlockStoreFactory.h
/// Abstract BlockStoreFactory Interface.

#include <string>

#include "utpexp.h"
#include "utpfwd.h"

#include "Except.h"
#include "RC.h"
#include "Types.h"

namespace utp {

class UTP_EXP BlockStoreFactory
{
public:
    /// Called by module init to register factory object.
    static void register_factory(std::string const & i_name,
                                 BlockStoreFactory * i_bsfp);

    /// Called at runtime to create new blockstore.
    ///
    /// @param[in] i_name Factory module name.
    /// @param[in] i_size Total size of the block store data in bytes.
    /// @param[in] i_args Implementation specific arguments.
    ///
    static BlockStoreHandle create(std::string const & i_name,
                                   size_t i_size,
                                   StringSeq const & i_args);

    /// Called at runtime to open existing blockstore.
    ///
    /// @param[in] i_name Factory module name.
    /// @param[in] i_args Implementation specific arguments.
    ///
    static BlockStoreHandle open(std::string const & i_name,
                                 StringSeq const & i_args);

    virtual ~BlockStoreFactory();

    virtual BlockStoreHandle bsf_create(size_t i_size,
                                        StringSeq const & i_args) = 0;

    virtual BlockStoreHandle bsf_open(StringSeq const & i_args) = 0;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_BlockStoreFactory_h__
