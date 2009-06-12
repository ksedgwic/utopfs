#include <sstream>
#include <string>

#include <ace/Get_Opt.h>

#include "Log.h"
#include "Types.h"

#include "FSBlockStore.h"

#include "FSBSFactory.h"
#include "fsbslog.h"

using namespace std;
using namespace utp;

FSBSFactory::FSBSFactory()
{
}

FSBSFactory::~FSBSFactory()
{
}

int
FSBSFactory::init(int argc, char * argv[])
{
    BlockStoreFactory::register_factory("FSBS", this);

    return 0;
}

int
FSBSFactory::fini()
{
    // Don't try to log here.  We are called during static
    // destruction and the log categories may have already
    // been removed.
    return 0;
}

int
FSBSFactory::suspend()
{
    return 0;
}

int
FSBSFactory::resume()
{
    return 0;
}

BlockStoreHandle
FSBSFactory::bsf_create(StringSeq const & i_args)
{
    BlockStoreHandle bsh = new FSBS::FSBlockStore();
    bsh->bs_create(i_args[0]);
    return bsh;
}

BlockStoreHandle
FSBSFactory::bsf_open(StringSeq const & i_args)
{
    BlockStoreHandle bsh = new FSBS::FSBlockStore();
    bsh->bs_open(i_args[0]);
    return bsh;
}

ACE_SVC_FACTORY_DEFINE(FSBSFactory)

ACE_STATIC_SVC_DEFINE(FSBSFactory,
                      ACE_TEXT ("FSBSFactory"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME (FSBSFactory),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
