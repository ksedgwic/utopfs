#include "BlockStore.h"

namespace {

utp::BlockStoreHandle g_bsh;    

} // end namespace

namespace utp {

void
BlockStore::instance(BlockStoreHandle const & i_bsh)
{
    g_bsh = i_bsh;
}

BlockStoreHandle
BlockStore::instance()
{
    return g_bsh;
}

BlockStore::~BlockStore()
{
}

} // end namespace utp
