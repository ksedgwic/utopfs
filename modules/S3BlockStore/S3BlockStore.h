#ifndef S3BlockStore_h__
#define S3BlockStore_h__

/// @file S3BlockStore.h
/// FileSystem BlockStore Instance.

#include <string>
#include <set>
#include <list>

#include <libs3.h>

#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "BlockStore.h"
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

class S3BS_EXP S3BlockStore : public utp::BlockStore
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

    static std::string blockpath(std::string const & i_entry = "");
    
    static std::string edgepath(std::string const & i_entry = "");

protected:
    static void parse_params(utp::StringSeq const & i_args,
                             S3Protocol & o_protocol,
                             S3UriStyle & o_uri_style,
                             std::string & o_access_key_id,
                             std::string & o_secret_access_key,
                             std::string & o_bucket_name);

    void setup_params(utp::StringSeq const & i_args);
    
    std::string entryname(void const * i_keydata, size_t i_keysize) const;

    std::string ridname(utp::uint64 i_rid) const;

    bool touch_entry(std::string const & i_entry,
                     time_t i_tstamp,
                     off_t i_size,
                     bool i_plsinsert);

    void purge_uncommitted();

    void write_head(utp::SignedHeadEdge const & i_she);

private:
    static bool		    		c_s3inited;

    std::string					m_instname;

    S3Protocol					m_protocol;
    S3UriStyle					m_uri_style;
    std::string					m_access_key_id;
    std::string					m_secret_access_key;
    std::string					m_bucket_name;

    utp::LameHeadNodeGraph		m_lhng;

    mutable ACE_Thread_Mutex	m_s3bsmutex;

    off_t						m_size;       // Total Size in Bytes
    off_t						m_committed;  // Committed Bytes (must be saved)
    off_t						m_uncommitted;// Uncommitted Bytes (reclaimable)

    EntrySet					m_entries;
    EntryList					m_lru;        // front=newest, back=oldest

    std::string					m_markname;
    EntryHandle					m_mark;
};

// FIXME - Why can't I use the one in utp::BlockStore?
// Helpful for debugging.
std::ostream & operator<<(std::ostream & ostrm, utp::HeadNode const & i_nr);

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // S3BlockStore_h__
