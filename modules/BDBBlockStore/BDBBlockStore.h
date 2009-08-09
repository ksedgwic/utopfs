#ifndef BDBBlockStore_h__
#define BDBBlockStore_h__

/// @file BDBBlockStore.h
/// Berkeley Database BlockStore Instance.

#include <string>

#include "utpfwd.h"
#include "BlockStore.h"
#include "bdbbsexp.h"



class Db;
class DbEnv;

namespace BDBBS {

class BDBBS_EXP BDBBlockStore : public utp::BlockStore
{
public:

    static void destroy(utp::StringSeq const & i_args);

    BDBBlockStore();

    virtual ~BDBBlockStore();

    // BlockStore methods.
    virtual void bs_create(size_t i_size, utp::StringSeq const & i_args)
        throw(utp::NotUniqueError,
              utp::InternalError,
              utp::ValueError);

    virtual void bs_open(utp::StringSeq const & i_args)
        throw(utp::InternalError,
              utp::NotFoundError);

    virtual void bs_close()
        throw(utp::InternalError);

    virtual void bs_stat(Stat & o_stat)
        throw(utp::InternalError);

    virtual void bs_get_block_async(void const * i_keydata,
                                    size_t i_keysize,
                                    void * o_outbuff,
                                    size_t i_outsize,
                                    BlockGetCompletion & i_cmpl)
        throw(utp::InternalError,
              utp::ValueError);

    virtual void bs_put_block_async(void const * i_keydata,
                                    size_t i_keysize,
                                    void const * i_blkdata,
                                    size_t i_blksize,
                                    BlockPutCompletion & i_cmpl)
        throw(utp::InternalError,
              utp::ValueError);

    virtual void bs_refresh_start(utp::uint64 i_rid)
        throw(utp::InternalError,
              utp::NotUniqueError);

    virtual void bs_refresh_block_async(utp::uint64 i_rid,
                                        void const * i_keydata,
                                        size_t i_keysize,
                                        BlockRefreshCompletion & i_cmpl)
        throw(utp::InternalError,
              utp::NotFoundError);
        
    virtual void bs_refresh_finish(utp::uint64 i_rid)
        throw(utp::InternalError,
              utp::NotFoundError);

	virtual void bs_sync()
		throw(utp::InternalError);
	
    virtual void bs_head_insert_async(utp::SignedHeadNode const & i_shn,
                                      SignedHeadInsertCompletion & i_cmpl)
        throw(utp::InternalError);

    virtual void bs_head_follow_async(utp::SignedHeadNode const & i_seed,
                                      SignedHeadTraverseFunc & i_func)
        throw(utp::InternalError);

    virtual void bs_head_furthest_async(utp::SignedHeadNode const & i_seed,
                                        SignedHeadTraverseFunc & i_func)
        throw(utp::InternalError);

protected:

	bool m_db_opened;
	DbEnv *m_dbe;
	Db *m_db;
	Db *m_db_refresh_ids; 	//refresh_id (unique,btree) => timestamp
	Db *m_db_refresh_entries; //refresh_id (non-unique, btree)=> key

    std::string m_rootpath;
    
    std::string get_full_path(void const * i_keydata,
                                size_t i_keysize);

	void open_dbs(u_int32_t addl_flags)
			throw(utp::InternalError);

};

} // namespace BDBBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // BDBBlockStore_h__
