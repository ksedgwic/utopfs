#include <sstream>
#include <string>

#include <ace/Get_Opt.h>

#include "Log.h"
#include "Types.h"

#include "VBlockStore.h"

#include "VBSFactory.h"
#include "vbslog.h"

using namespace std;
using namespace utp;

VBSFactory::VBSFactory()
{
}

VBSFactory::~VBSFactory()
{
}

int
VBSFactory::init(int argc, char * argv[])
{
    BlockStoreFactory::register_factory("VBS", this);

    return 0;
}

int
VBSFactory::fini()
{
    // Don't try to log here.  We are called during static
    // destruction and the log categories may have already
    // been removed.
    return 0;
}

int
VBSFactory::suspend()
{
    return 0;
}

int
VBSFactory::resume()
{
    return 0;
}

BlockStoreHandle
VBSFactory::bsf_create(string const & i_instname,
                       size_t i_size,
                       StringSeq const & i_args)
    throw(InternalError,
          NotUniqueError,
          ValueError)
{
    BlockStoreHandle bsh = new VBS::VBlockStore(i_instname);
    bsh->bs_create(i_size, i_args);
    return bsh;
}

BlockStoreHandle
VBSFactory::bsf_open(string const & i_instname,
                     StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
    BlockStoreHandle bsh = new VBS::VBlockStore(i_instname);
    bsh->bs_open(i_args);
    return bsh;
}

void
VBSFactory::bsf_destroy(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError,
          ValueError)
{
    VBS::VBlockStore::destroy(i_args);
}

ACE_SVC_FACTORY_DEFINE(VBSFactory)

ACE_STATIC_SVC_DEFINE(VBSFactory,
                      ACE_TEXT ("VBSFactory"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME (VBSFactory),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
