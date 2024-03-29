#ifndef S3BlockStore_h__
#define S3BlockStore_h__

/// @file S3BlockStore.h
/// FileSystem BlockStore Instance.

#include <iosfwd>
#include <list>
#include <set>
#include <string>

#include <libs3.h>

#include <ace/Event_Handler.h>
#include <ace/Handle_Set.h>
#include <ace/Reactor.h>
#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "BlockStore.h"
#include "S3ResponseHandler.h"
#include "LameHeadNodeGraph.h"
#include "RC.h"

#include "s3bsexp.h"


namespace S3BS {

class Entry;
typedef utp::RCPtr<Entry> EntryHandle;
typedef std::list<EntryHandle> EntryList;

class S3BS_EXP Entry : public utp::RCObj
{
public:
    Entry(std::string const & i_name, time_t i_tstamp, off_t i_size)
        : m_name(i_name) , m_tstamp(i_tstamp) , m_size(i_size) {}

    std::string				m_name;
    time_t					m_tstamp;
    off_t					m_size;
    EntryList::iterator		m_listpos;
};
typedef utp::RCPtr<Entry> EntryHandle;

// Comparison functor by name.
struct lessByName {
    bool operator()(EntryHandle const & i_a, EntryHandle const & i_b) {
        return i_a->m_name < i_b->m_name;
    }
};

// Comparison functor by tstamp.
struct lessByTstamp {
    bool operator()(EntryHandle const & i_a, EntryHandle const & i_b) {
        return i_a->m_tstamp < i_b->m_tstamp;
    }
};

class S3BS_EXP S3BlockStore
    : public utp::BlockStore
    , public ACE_Event_Handler
{
public:
    typedef std::set<EntryHandle, lessByName> EntrySet;
    typedef std::multiset<EntryHandle, lessByTstamp> EntryTimeSet;

    static void destroy(utp::StringSeq const & i_args);

    S3BlockStore(std::string const & i_instname);

    virtual ~S3BlockStore();

    // BlockStore methods.

    virtual std::string const & bs_instname() const;

    virtual void bs_create(size_t i_size,
                           utp::StringSeq const & i_args)
        throw(utp::NotUniqueError,
              utp::InternalError,
              utp::ValueError);

    virtual void bs_open(utp::StringSeq const & i_args)
        throw(utp::InternalError,
              utp::NotFoundError,
              utp::ValueError);

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

    virtual void bs_head_insert_async(utp::SignedHeadEdge const & i_shn,
                                      HeadEdgeInsertCompletion & i_cmpl,
                                      void const * i_argp)
        throw(utp::InternalError);

    virtual void bs_head_follow_async(utp::HeadNode const & i_hn,
                                      HeadEdgeTraverseFunc & i_func,
                                      void const * i_argp)
        throw(utp::InternalError);

    virtual void bs_head_furthest_async(utp::HeadNode const & i_hn,
                                        HeadNodeTraverseFunc & i_func,
                                        void const * i_argp)
        throw(utp::InternalError);

    virtual void bs_get_stats(utp::StatSet & o_ss) const
        throw(utp::InternalError);

    virtual bool bs_issaturated()
        throw(utp::InternalError);

    virtual void bs_register_unsathandler(UnsaturatedHandler * i_handler,
                                          void const * i_argp)
        throw(utp::InternalError);

    // ACE_Event_Handler methods

    virtual int handle_input(ACE_HANDLE i_fd);

    virtual int handle_output(ACE_HANDLE i_fd);

    virtual int handle_exception(ACE_HANDLE i_fd);

    virtual int handle_timeout(ACE_Time_Value const & current_time,
                               void const * act);

    // S3BlockStore methods

    static std::string blockpath(std::string const & i_entry = "");
    
    static std::string edgepath(std::string const & i_entry = "");

    void remove_handler(ResponseHandlerHandle const & i_rhh);

    void initiate_get(AsyncGetHandlerHandle const & i_aghh);

    void initiate_put(AsyncPutHandlerHandle const & i_aphh);

    void update_put_stats(AsyncPutHandlerHandle const & i_aphh);

protected:
    static void parse_params(utp::StringSeq const & i_args,
                             S3Protocol & o_protocol,
                             S3UriStyle & o_uri_style,
                             std::string & o_access_key_id,
                             std::string & o_secret_access_key,
                             std::string & o_bucket_name,
                             std::string & o_mdndx_path_name);

    void initiate_get_internal(AsyncGetHandlerHandle const & i_aghh);

    void initiate_put_internal(AsyncPutHandlerHandle const & i_aphh);

    int reqctxt_service();

    void reqctxt_reregister();

    void setup_params(utp::StringSeq const & i_args);
    
    std::string entryname(void const * i_keydata, size_t i_keysize) const;

    std::string ridname(utp::uint64 i_rid) const;

    bool touch_entry(std::string const & i_entry,
                     time_t i_tstamp,
                     off_t i_size,
                     bool i_plsinsert);

    void purge_uncommitted();

    void write_head(utp::SignedHeadEdge const & i_she);

    void parse_mdndx_entries(std::istream & i_strm, EntryTimeSet & o_ets);

private:
    static bool		    		c_s3inited;

    ACE_Reactor *				m_reactor;

    std::string					m_instname;

    S3Protocol					m_protocol;
    S3UriStyle					m_uri_style;
    std::string					m_access_key_id;
    std::string					m_secret_access_key;
    std::string					m_bucket_name;
    std::string					m_mdndx_path_name;

    S3BucketContext				m_buckctxt;

    utp::LameHeadNodeGraph		m_lhng;

    mutable ACE_Thread_Mutex	m_s3bsmutex;
    ACE_Condition_Thread_Mutex	m_s3bscond;

    bool						m_waiting;

    S3RequestContext *			m_reqctxt;

    ACE_Handle_Set 				m_rset;
    ACE_Handle_Set				m_wset;
    ACE_Handle_Set				m_eset;

    ResponseHandlerSeq			m_rsphandlers;

    off_t						m_size;       // Total Size in Bytes
    off_t						m_committed;  // Committed Bytes (must be saved)
    off_t						m_uncommitted;// Uncommitted Bytes (reclaimable)

    EntrySet					m_entries;
    EntryList					m_lru;        // front=newest, back=oldest

    std::string					m_markname;
    EntryHandle					m_mark;

    utp::BlockStore::UnsaturatedHandler *		m_unsathandler;
    void const *								m_unsatargp;
};

// FIXME - Why can't I use the one in utp::BlockStore?
// Helpful for debugging.
std::ostream & operator<<(std::ostream & ostrm, utp::HeadNode const & i_nr);

std::ostream & operator<<(std::ostream & ostrm, S3Status status);

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // S3BlockStore_h__
