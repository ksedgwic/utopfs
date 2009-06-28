#include <sstream>
#include <string>

#include <ace/Get_Opt.h>

#include "Log.h"
#include "Types.h"

#include "BDBBlockStore.h"

#include "BDBBSFactory.h"
#include "bdbbslog.h"

using namespace std;
using namespace utp;

BDBBSFactory::BDBBSFactory()
{
}

BDBBSFactory::~BDBBSFactory()
{
}

int
BDBBSFactory::init(int argc, char * argv[])
{
    BlockStoreFactory::register_factory("BDBBS", this);

    return 0;
}

int
BDBBSFactory::fini()
{
    // Don't try to log here.  We are called during static
    // destruction and the log categories may have already
    // been removed.
    return 0;
}

int
BDBBSFactory::suspend()
{
    return 0;
}

int
BDBBSFactory::resume()
{
    return 0;
}

BlockStoreHandle
BDBBSFactory::bsf_create(StringSeq const & i_args)
{
    BlockStoreHandle bsh = new BDBBS::BDBBlockStore();
    bsh->bs_create(i_args[0]);
    return bsh;
}

BlockStoreHandle
BDBBSFactory::bsf_open(StringSeq const & i_args)
{
    BlockStoreHandle bsh = new BDBBS::BDBBlockStore();
    bsh->bs_open(i_args[0]);
    return bsh;
}

ACE_SVC_FACTORY_DEFINE(BDBBSFactory)

ACE_STATIC_SVC_DEFINE(BDBBSFactory,
                      ACE_TEXT ("BDBBSFactory"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME (BDBBSFactory),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
