#ifndef S3BucketDestroyer_h__
#define S3BucketDestroyer_h__

/// @file S3BucketDestroyer.h
/// FileSystem S3 Bucket Destroyer.

#include <queue>

#include <ace/Event_Handler.h>
#include <ace/Reactor.h>
#include <ace/Thread_Mutex.h>

#include <libs3.h>

#include "utpfwd.h"
#include "BlockStore.h"
#include "Types.h"
#include "RC.h"

#include "S3ResponseHandler.h"
#include "s3bsexp.h"
#include "s3bsfwd.h"

namespace S3BS {

class BucketDestroyer;

class ObjectDestroyer
    : public ResponseHandler
    , public ACE_Event_Handler
{
public:
    ObjectDestroyer(BucketDestroyer & i_bd,
                    std::string const & i_key,
                    size_t i_retries);

    virtual ~ObjectDestroyer();

    // ResponseHandler methods

    virtual void rh_complete(S3Status status,
                             S3ErrorDetails const * errorDetails);

    // ACE_Event_Handler methods

    virtual Reference_Count add_reference();

    virtual Reference_Count remove_reference();

    virtual int handle_exception(ACE_HANDLE fd);

    // ObjectDestroyer methods

    std::string const & key() const { return m_key; }

private:
    BucketDestroyer &		m_bd;
    std::string				m_key;
    ACE_Reactor *			m_reactor;
    size_t					m_retries;
    ObjectDestroyerHandle	m_self;
};

// NOTE - Unfortunately this class duplicates a lot of the features in
// the S3BlockStore object.  But it is used when we don't actually
// want to create a blockstore instance (we are destroying one) ...

class BucketDestroyer
    : public ListHandler
    , public ACE_Event_Handler
{
public:
    BucketDestroyer(S3BucketContext & i_buckctxt);

    virtual ~BucketDestroyer();

    virtual S3Status lh_item(int i_istrunc,
                             char const * i_next_marker,
                             int i_contents_count,
                             S3ListBucketContent const * i_contents,
                             int i_common_prefixes_count,
                             char const ** i_common_prefixes);

    virtual int handle_input(ACE_HANDLE i_fd);

    virtual int handle_output(ACE_HANDLE i_fd);

    virtual int handle_exception(ACE_HANDLE i_fd);

    virtual int handle_timeout(ACE_Time_Value const & current_time,
                               void const * act);

    void initiate_delete(ObjectDestroyerHandle const & i_odh);

    int reqctxt_service();

    void reqctxt_reregister();

    void completed();

    bool m_istrunc;
    std::string m_last_seen;

protected:
    void service_queue();

private:
    typedef std::queue<std::string> KeyQueue;

    S3BucketContext &			m_buckctxt;
    ACE_Reactor *				m_reactor;
    mutable ACE_Thread_Mutex	m_bdmutex;
    S3RequestContext *			m_reqctxt;
    ACE_Handle_Set 				m_rset;
    ACE_Handle_Set				m_wset;
    ACE_Handle_Set				m_eset;
    size_t						m_outstanding;
    KeyQueue					m_keyqueue;
};

} // namespace S3BS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // S3BucketDestroyer_h__
