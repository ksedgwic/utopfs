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

    /// Create new blockstore.
    ///
    /// @param[in] i_factname Factory module name.
    /// @param[in] i_instname BlockStore instance name.
    /// @param[in] i_size Total size of the block store data in bytes.
    /// @param[in] i_args Implementation specific arguments.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified factory module was not found.
    /// @throw NotUniqueError The specified blockstore instance already exists.
    ///
    static BlockStoreHandle create(std::string const & i_factname,
                                   std::string const & i_instname,
                                   size_t i_size,
                                   StringSeq const & i_args)
        throw(InternalError,
              NotFoundError,
              NotUniqueError);

    /// Open existing blockstore.
    ///
    /// @param[in] i_factname Factory module name.
    /// @param[in] i_instname BlockStore instance name.
    /// @param[in] i_args Implementation specific arguments.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified blockstore factory was not found.
    /// @throw NotUniqueError The specified blockstore instance already exists.
    ///
    static BlockStoreHandle open(std::string const & i_factname,
                                 std::string const & i_instname,
                                 StringSeq const & i_args)
        throw(InternalError,
              NotFoundError,
              NotUniqueError);

    /// Find open blockstore.
    ///
    /// @param[in] i_instname BlockStore instance name.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified blockstore was not found.
    ///
    static BlockStoreHandle find(std::string const & i_instname)
        throw(InternalError,
              NotFoundError);

    /// Remove blockstore from open list.
    ///
    /// @param[in] i_instname BlockStore instance name.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw NotFoundError The specified blockstore was not found.
    ///
    static void remove(std::string const & i_instname)
        throw(InternalError,
              NotFoundError);

    /// Remove existing blockstore.
    ///
    /// NOTE - This should not be called when the blockstore is open;
    /// it removes any persistent storage associated with the
    /// blockstore.
    ///
    /// @param[in] i_name Factory module name.
    /// @param[in] i_args Implementation specific arguments.
    ///
    /// @throw InternalError An non-recoverable error occurred.
    /// @throw ValueError The specified factory module was not found.
    /// @throw NotFoundError The specified blockstore was not found.
    ///
    static void destroy(std::string const & i_name,
                        StringSeq const & i_args)
        throw(InternalError,
              ValueError,
              NotFoundError);

    virtual ~BlockStoreFactory();

    virtual BlockStoreHandle bsf_create(std::string const & i_instname,
                                        size_t i_size,
                                        StringSeq const & i_args)
        throw(InternalError,
              NotUniqueError) = 0;

    virtual BlockStoreHandle bsf_open(std::string const & i_instname,
                                      StringSeq const & i_args)
        throw(InternalError,
              NotFoundError) = 0;

    virtual void bsf_destroy(StringSeq const & i_args)
        throw(InternalError,
              NotFoundError) = 0;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_BlockStoreFactory_h__
