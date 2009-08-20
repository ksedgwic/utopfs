#ifndef VBlockStore_h__
#define VBlockStore_h__

/// @file VBlockStore.h
/// Virtual BlockStore.

#include <map>
#include <set>
#include <string>

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "BlockStore.h"
#include "RC.h"

#include "vbsexp.h"
#include "vbsfwd.h"

namespace VBS {

// The Virtual BlockStore.
//
class VBS_EXP VBlockStore : public utp::BlockStore
{
public:
    static void destroy(utp::StringSeq const & i_args);

    VBlockStore(std::string const & i_instname);

    virtual ~VBlockStore();

    // BlockStore methods.

    virtual std::string const & bs_instname() const;

    virtual void bs_create(size_t i_size,
                           utp::StringSeq const & i_args)
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

    virtual void bs_sync()
		throw(utp::InternalError);

     virtual void bs_block_get_async(void const * i_keydata,
                                    size_t i_keysize,
                                    void * o_outbuff,
                                    size_t i_outsize,
                                    BlockGetCompletion & i_cmpl,
                                    void const * i_argp)
        throw(utp::InternalError,
              utp::ValueError);

    virtual void bs_block_put_async(void const * i_keydata,
                                    size_t i_keysize,
                                    void const * i_blkdata,
                                    size_t i_blksize,
                                    BlockPutCompletion & i_cmpl,
                                    void const * i_argp)
        throw(utp::InternalError,
              utp::ValueError);

    virtual void bs_refresh_start_async(utp::uint64 i_rid,
                                        RefreshStartCompletion & i_cmpl,
                                        void const * i_argp)
        throw(utp::InternalError);

    virtual void bs_refresh_block_async(utp::uint64 i_rid,
                                        void const * i_keydata,
                                        size_t i_keysize,
                                        RefreshBlockCompletion & i_cmpl,
                                        void const * i_argp)
        throw(utp::InternalError,
              utp::NotFoundError);
        
    virtual void bs_refresh_finish_async(utp::uint64 i_rid,
                                        RefreshFinishCompletion & i_cmpl,
                                        void const * i_argp)
        throw(utp::InternalError);

    virtual void bs_head_insert_async(utp::SignedHeadNode const & i_shn,
                                      SignedHeadInsertCompletion & i_cmpl,
                                      void const * i_argp)
        throw(utp::InternalError);

    virtual void bs_head_follow_async(utp::SignedHeadNode const & i_seed,
                                      SignedHeadTraverseFunc & i_func,
                                      void const * i_argp)
        throw(utp::InternalError);

    virtual void bs_head_furthest_async(utp::SignedHeadNode const & i_seed,
                                        SignedHeadTraverseFunc & i_func,
                                        void const * i_argp)
        throw(utp::InternalError);

    // VBS Methods

    void insert_request(VBSRequestHandle const & i_rh);

    void remove_request(VBSRequestHandle const & i_rh);

protected:

private:
    typedef std::map<std::string, VBSChildHandle> VBSChildMap;
    typedef std::set<VBSRequestHandle> VBSRequestSet;

    std::string					m_instname;
    VBSChildMap					m_children;

    ACE_Thread_Mutex			m_vbsmutex;
    ACE_Condition_Thread_Mutex	m_vbscond;
    bool						m_waiting;
    VBSRequestSet				m_requests;
};

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBlockStore_h__
