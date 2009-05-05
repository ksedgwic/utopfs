#include "AssocStore.h"

namespace {

utp::AssocStoreHandle g_ash;    

} // end namespace

namespace utp {

void
AssocStore::instance(AssocStoreHandle const & i_ash)
{
    g_ash = i_ash;
}

AssocStoreHandle
AssocStore::instance()
{
    return g_ash;
}

AssocStore::~AssocStore()
{
}

} // end namespace utp
