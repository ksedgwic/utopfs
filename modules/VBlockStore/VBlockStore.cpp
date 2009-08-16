#include "Log.h"
#include "BlockStoreFactory.h"

#include "VBlockStore.h"
#include "vbslog.h"

using namespace std;
using namespace utp;

namespace VBS {

void
VBlockStore::destroy(StringSeq const & i_args)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::destroy unimplemented");
}

VBlockStore::VBlockStore(string const & i_instname)
    : m_instname(i_instname)
{
    LOG(lgr, 4, m_instname << ' ' << "CTOR");
}

VBlockStore::~VBlockStore()
{
}

void
VBlockStore::bs_create(size_t i_size, StringSeq const & i_args)
    throw(NotUniqueError,
          InternalError,
          ValueError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_create unimplemented");
}

void
VBlockStore::bs_open(StringSeq const & i_args)
    throw(InternalError,
          NotFoundError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_open unimplemented");
}

void
VBlockStore::bs_close()
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_close unimplemented");

    // Unregister this instance.
    try
    {
        BlockStoreFactory::remove(m_instname);
    }
    catch (InternalError const & ex)
    {
        // This we can just rethrow ...
        throw;
    }
    catch (Exception const & ex)
    {
        // These shouldn't happen and need to be converted to
        // InternalError ...
        throw InternalError(ex.what());
    }
}

void
VBlockStore::bs_stat(Stat & o_stat)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_stat unimplemented");
}

void
VBlockStore::bs_get_block_async(void const * i_keydata,
                                 size_t i_keysize,
                                 void * o_buffdata,
                                 size_t i_buffsize,
                                 BlockGetCompletion & i_cmpl)
    throw(InternalError,
          ValueError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_get_block_async unimplemented");
}

void
VBlockStore::bs_put_block_async(void const * i_keydata,
                                 size_t i_keysize,
                                 void const * i_blkdata,
                                 size_t i_blksize,
                                 BlockPutCompletion & i_cmpl)
    throw(InternalError,
          ValueError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_put_block_async unimplemented");
}

void
VBlockStore::bs_refresh_start(uint64 i_rid)
    throw(InternalError,
          NotUniqueError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_refresh_start unimplemented");
}

void
VBlockStore::bs_refresh_block_async(uint64 i_rid,
                                     void const * i_keydata,
                                     size_t i_keysize,
                                     BlockRefreshCompletion & i_cmpl)
    throw(InternalError,
          NotFoundError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_refresh_block_async unimplemented");
}
        
void
VBlockStore::bs_refresh_finish(uint64 i_rid)
    throw(InternalError,
          NotFoundError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_refresh_finish unimplemented");
}

void
VBlockStore::bs_sync()
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_sync unimplemented");
}

void
VBlockStore::bs_head_insert_async(SignedHeadNode const & i_shn,
                                   SignedHeadInsertCompletion & i_cmpl)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_head_insert_async unimplemented");
}

void
VBlockStore::bs_head_follow_async(SignedHeadNode const & i_shn,
                                   SignedHeadTraverseFunc & i_func)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_head_follow_async unimplemented");
}

void
VBlockStore::bs_head_furthest_async(SignedHeadNode const & i_shn,
                                     SignedHeadTraverseFunc & i_func)
    throw(InternalError)
{
    throwstream(InternalError, FILELINE
                << "VBlockStore::bs_head_furthest_async unimplemented");
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
