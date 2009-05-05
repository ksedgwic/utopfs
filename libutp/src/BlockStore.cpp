#include "BlockStore.h"

namespace {

utp::BlockStoreHandle g_ash;    

} // end namespace

namespace utp {

void
BlockStore::instance(BlockStoreHandle const & i_ash)
{
    g_ash = i_ash;
}

BlockStoreHandle
BlockStore::instance()
{
    return g_ash;
}

BlockStore::~BlockStore()
{
}

} // end namespace utp
