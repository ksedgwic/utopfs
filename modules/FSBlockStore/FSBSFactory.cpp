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
FSBSFactory::bsf_create(string const & i_instname,
                        size_t i_size,
                        StringSeq const & i_args)
    throw(InternalError,
          NotUniqueError,
          ValueError)
{
    BlockStoreHandle bsh = new FSBS::FSBlockStore(i_instname);
    bsh->bs_create(i_size, i_args);
    return bsh;
}

BlockStoreHandle
FSBSFactory::bsf_open(string const & i_instname,
                      StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
    BlockStoreHandle bsh = new FSBS::FSBlockStore(i_instname);
    bsh->bs_open(i_args);
    return bsh;
}

void
FSBSFactory::bsf_destroy(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
    FSBS::FSBlockStore::destroy(i_args);
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
