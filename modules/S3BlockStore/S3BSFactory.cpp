#include <sstream>
#include <string>

#include <ace/Get_Opt.h>

#include "Log.h"
#include "Types.h"

#include "S3BlockStore.h"

#include "S3BSFactory.h"
#include "s3bslog.h"

using namespace std;
using namespace utp;

S3BSFactory::S3BSFactory()
{
}

S3BSFactory::~S3BSFactory()
{
}

int
S3BSFactory::init(int argc, char * argv[])
{
    BlockStoreFactory::register_factory("S3BS", this);

    return 0;
}

int
S3BSFactory::fini()
{
    // Don't try to log here.  We are called during static
    // destruction and the log categories may have already
    // been removed.
    return 0;
}

int
S3BSFactory::suspend()
{
    return 0;
}

int
S3BSFactory::resume()
{
    return 0;
}

BlockStoreHandle
S3BSFactory::bsf_create(string const & i_instname,
                        size_t i_size,
                        StringSeq const & i_args)
    throw(InternalError,
          NotUniqueError)
{
    BlockStoreHandle bsh = new S3BS::S3BlockStore(i_instname);
    bsh->bs_create(i_size, i_args);
    return bsh;
}

BlockStoreHandle
S3BSFactory::bsf_open(string const & i_instname,
                      StringSeq const & i_args)
    throw(InternalError,
          NotFoundError)
{
    BlockStoreHandle bsh = new S3BS::S3BlockStore(i_instname);
    bsh->bs_open(i_args);
    return bsh;
}

void
S3BSFactory::bsf_destroy(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError)
{
    S3BS::S3BlockStore::destroy(i_args);
}

ACE_SVC_FACTORY_DEFINE(S3BSFactory)

ACE_STATIC_SVC_DEFINE(S3BSFactory,
                      ACE_TEXT ("S3BSFactory"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME (S3BSFactory),
                      ACE_Service_Type::DELETE_THIS |
                      ACE_Service_Type::DELETE_OBJ,
                      0)

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
