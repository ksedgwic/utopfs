#include <map>

#include "Log.h"
#include "Except.h"
#include "BlockStore.h"
#include "BlockStoreFactory.h"

#include "utplog.h"

using namespace std;
using namespace utp;

namespace {

typedef std::map<std::string, utp::BlockStoreFactory *> BlockStoreFactoryMap;
typedef std::map<std::string, utp::BlockStoreHandle> BlockStoreInstanceMap;

ACE_Thread_Mutex		g_bsfmutex;
BlockStoreFactoryMap	g_bsfm;
BlockStoreInstanceMap	g_bsim;

BlockStoreFactory *
find_factory(string const & i_name)
{
    BlockStoreFactoryMap::iterator pos = g_bsfm.find(i_name);
    if (pos == g_bsfm.end())
        throwstream(NotFoundError,
                    "blockstore factory for \"" << i_name << "\" not found");

    return pos->second;
}

} // end namespace

namespace utp {

void
BlockStoreFactory::register_factory(string const & i_factname,
                                    BlockStoreFactory * i_bsfp)
{
    LOG(lgr, 4, "register_factory " << i_factname);

    g_bsfm[i_factname] = i_bsfp;
}

BlockStoreHandle
BlockStoreFactory::create(string const & i_factname,
                          string const & i_instname,
                          size_t i_size,
                          StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          NotUniqueError)
{
    LOG(lgr, 4, "create"
        << ' ' << i_factname
        << ' ' << i_instname
        << ' ' << i_size);

    {
        ACE_Guard<ACE_Thread_Mutex> guard(g_bsfmutex);

        // Does this instance already exist?
        BlockStoreInstanceMap::iterator pos = g_bsim.find(i_instname);
        if (pos != g_bsim.end())
            throwstream(NotUniqueError,
                        "blockstore instance for \"" << i_instname
                        << "\" already exists");

        // Reserve the name with a placeholder.
        g_bsim.insert(make_pair(i_instname, BlockStoreHandle(NULL)));
    }

    // Unlock the mutex while we create the blockstore ...

    try
    {
        BlockStoreHandle bsh =
            find_factory(i_factname)->bsf_create(i_instname, i_size, i_args);

        // Replace the placeholder w/ the real thing.
        ACE_Guard<ACE_Thread_Mutex> guard(g_bsfmutex);
        g_bsim[i_instname] = bsh;

        // And return it ...
        return bsh;
    }
    catch (...)
    {
        // Remove the placeholder.
        ACE_Guard<ACE_Thread_Mutex> guard(g_bsfmutex);
        g_bsim.erase(i_instname);

        throw;	// Rethrow the exception.
    }
}
                          
BlockStoreHandle
BlockStoreFactory::open(string const & i_factname,
                        string const & i_instname,
                        StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          NotUniqueError)
{
    LOG(lgr, 4, "open"
        << ' ' << i_factname
        << ' ' << i_instname);

    {
        ACE_Guard<ACE_Thread_Mutex> guard(g_bsfmutex);

        // Does this instance already exist?
        BlockStoreInstanceMap::iterator pos = g_bsim.find(i_instname);
        if (pos != g_bsim.end())
            throwstream(NotUniqueError,
                        "blockstore instance for \"" << i_instname
                        << "\" already exists");

        // Reserve the name with a placeholder.
        g_bsim.insert(make_pair(i_instname, BlockStoreHandle(NULL)));
    }

    // Unlock the mutex while we open the blockstore ...

    try
    {
        BlockStoreHandle bsh =
            find_factory(i_factname)->bsf_open(i_instname, i_args);

        // Replace the placeholder w/ the real thing.
        ACE_Guard<ACE_Thread_Mutex> guard(g_bsfmutex);
        g_bsim[i_instname] = bsh;

        // And return it ...
        return bsh;
    }
    catch (...)
    {
        // Remove the placeholder.
        ACE_Guard<ACE_Thread_Mutex> guard(g_bsfmutex);
        g_bsim.erase(i_instname);

        throw;	// Rethrow the exception.
    }
}

BlockStoreHandle
BlockStoreFactory::find(string const & i_instname)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 4, "find" << ' ' << i_instname);

    ACE_Guard<ACE_Thread_Mutex> guard(g_bsfmutex);

    // Does this instance exist?
    BlockStoreInstanceMap::iterator pos = g_bsim.find(i_instname);
    if (pos == g_bsim.end())
        throwstream(NotFoundError,
                    "blockstore instance \"" << i_instname << "\" not found");

    return pos->second;
}

void
BlockStoreFactory::remove(string const & i_instname)
    throw(InternalError,
          NotFoundError)
{
    LOG(lgr, 4, "remove" << ' ' << i_instname);

    ACE_Guard<ACE_Thread_Mutex> guard(g_bsfmutex);

    // Does this instance exist?
    BlockStoreInstanceMap::iterator pos = g_bsim.find(i_instname);
    if (pos == g_bsim.end())
        throwstream(NotFoundError,
                    "blockstore instance \"" << i_instname << "\" not found");

    g_bsim.erase(i_instname);
}
                          
void
BlockStoreFactory::destroy(string const & i_name, StringSeq const & i_args)
    throw(InternalError,
          ValueError,
          NotFoundError)
{
    LOG(lgr, 4, "destroy " << i_name);

    return find_factory(i_name)->bsf_destroy(i_args);
}
                          
BlockStoreFactory::~BlockStoreFactory()
{
}

} // end namespace utp
