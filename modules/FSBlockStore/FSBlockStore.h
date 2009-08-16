#ifndef FSBlockStore_h__
#define FSBlockStore_h__

/// @file FSBlockStore.h
/// FileSystem BlockStore Instance.

#include <fstream>
#include <list>
#include <set>
#include <string>

#include <ace/Thread_Mutex.h>

#include "utpfwd.h"

#include "BlockStore.h"
#include "RC.h"

#include "fsbsexp.h"

namespace FSBS {

class Entry;
typedef utp::RCPtr<Entry> EntryHandle;
typedef std::list<EntryHandle> EntryList;

class FSBS_EXP Entry : public utp::RCObj
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

// A NodeRef consists of a filesystem tag and the rootref.
//
typedef std::pair<std::string, std::string> NodeRef;

// Helpful for debugging.
std::ostream & operator<<(std::ostream & ostrm, NodeRef const & i_nr);

// A Edge represents a connection between two nodes.  They're
// reference counted so they can be held in multiple collections.
//
struct FSBS_EXP Edge : public utp::RCObj
{
    NodeRef					m_prev;
    NodeRef					m_root;
    utp::SignedHeadNode		m_shn;

    Edge(utp::SignedHeadNode const & i_shn);
};

typedef utp::RCPtr<Edge> EdgeHandle;

// Helpful for debugging.
std::ostream & operator<<(std::ostream & ostrm, Edge const & i_e);

// The Filesystem-based BlockStore.
//
class FSBS_EXP FSBlockStore : public utp::BlockStore
{
public:
    static void destroy(utp::StringSeq const & i_args);

    FSBlockStore(std::string const & i_instname);

    virtual ~FSBlockStore();

    // BlockStore methods.

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
    std::string entryname(void const * i_keydata, size_t i_keysize) const;

    std::string ridname(utp::uint64 i_rid) const;

    std::string markname() const { return "MARK"; }

    std::string headspath() const;

    std::string blockpath(std::string const & i_entry) const;

    void touch_entry(std::string const & i_entry,
                     time_t i_tstamp,
                     off_t i_size);

    void purge_uncommitted();

    void insert_head(utp::SignedHeadNode const & i_shn);

    void write_head(utp::SignedHeadNode const & i_shn);

private:
    typedef std::set<EntryHandle, lessByName> EntrySet;
    typedef std::multiset<EntryHandle, lessByTstamp> EntryTimeSet;
    typedef std::multimap<NodeRef, EdgeHandle> EdgeMap;
    typedef std::set<NodeRef> NodeRefSet;

    std::string			m_instname;

    off_t				m_size;			// Total Size in Bytes
    off_t				m_committed;	// Committed Bytes (must be saved)
    off_t				m_uncommitted;	// Uncommitted Bytes (reclaimable)
    std::string			m_rootpath;
    std::string			m_blockspath;

    std::ofstream		m_headsstrm;

    ACE_Thread_Mutex	m_fsbsmutex;

    EntrySet			m_entries;
    EntryList			m_lru;			// front=newest, back=oldest

    EntryHandle			m_mark;

    EdgeMap				m_prevmap;
    EdgeMap				m_rootmap;
    NodeRefSet			m_roots;		// Roots of the graph
};

} // namespace FSBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // FSBlockStore_h__
