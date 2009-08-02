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

BlockStoreFactoryMap g_bsfm;

BlockStoreFactory *
find_factory(string const & i_name)
{
    BlockStoreFactoryMap::iterator pos = g_bsfm.find(i_name);
    if (pos == g_bsfm.end())
        throwstream(ValueError,
                    "blockstore factory for \"" << i_name << "\" not found");

    return pos->second;
}

} // end namespace

namespace utp {

void
BlockStoreFactory::register_factory(string const & i_name,
                                    BlockStoreFactory * i_bsfp)
{
    LOG(lgr, 4, "register_factory " << i_name);

    g_bsfm[i_name] = i_bsfp;
}

BlockStoreHandle
BlockStoreFactory::create(string const & i_name,
                          size_t i_size,
                          StringSeq const & i_args)
        throw(InternalError,
              ValueError,
              NotUniqueError)
{
    LOG(lgr, 4, "create " << i_name << ' ' << i_size);

    return find_factory(i_name)->bsf_create(i_size, i_args);
}
                          
BlockStoreHandle
BlockStoreFactory::open(string const & i_name, StringSeq const & i_args)
        throw(InternalError,
              ValueError,
              NotFoundError)
{
    LOG(lgr, 4, "open " << i_name);

    return find_factory(i_name)->bsf_open(i_args);
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
