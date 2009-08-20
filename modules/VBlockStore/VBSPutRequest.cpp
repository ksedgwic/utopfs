#include <iostream>

#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSPutRequest.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSPutRequest::VBSPutRequest(VBlockStore & i_vbs,
                             long i_outstanding,
                             void const * i_keydata,
                             size_t i_keysize,
                             void const * i_blkdata,
                             size_t i_blksize,
                             BlockStore::BlockPutCompletion & i_cmpl,
                             void const * i_argp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_key((uint8 const *) i_keydata, (uint8 const *) i_keydata + i_keysize)
    , m_blk((uint8 const *) i_blkdata, (uint8 const *) i_blkdata + i_blksize)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
{
    LOG(lgr, 6, "PUT @" << (void *) this << " CTOR");
}

VBSPutRequest::~VBSPutRequest()
{
    LOG(lgr, 6, "PUT @" << (void *) this << " DTOR");
}

void
VBSPutRequest::stream_insert(std::ostream & ostrm) const
{
    ostrm << "PUT @" << (void *) this;
}

void
VBSPutRequest::bp_complete(void const * i_keydata,
                           size_t i_keysize,
                           void const * i_argp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " bp_complete");

    bool do_complete = false;
    bool do_done = false;

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the first successful completion?
        if (!m_succeeded)
        {
            do_complete = true;
            m_succeeded = true;
        }

        // Are we the last completion?
        --m_outstanding;
        if (m_outstanding == 0)
            do_done = true;
    }

    // If we are the first child back with success we get
    // to tell the parent ...
    //
    if (do_complete)
        m_cmpl.bp_complete(&m_key[0], m_key.size(), m_argp);

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    if (do_done)
        done();
}

void
VBSPutRequest::bp_error(void const * i_keydata,
                        size_t i_keysize,
                        void const * i_argp,
                        Exception const & i_exp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " bp_error");

    bool do_complete = false;
    bool do_done = false;

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the last completion?
        --m_outstanding;
        if (m_outstanding == 0)
        {
            do_done = true;

            // If no other child succeeded send our status.
            if (!m_succeeded)
                do_complete = true;
        }
    }

    // If we are the last child back with an exception we
    // get to tell the parent ...
    //
    if (do_complete)
        m_cmpl.bp_error(&m_key[0], m_key.size(), m_argp, i_exp);

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    if (do_done)
        done();
}

void
VBSPutRequest::process(VBSChild * i_cp, BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " process");

    i_bsh->bs_put_block_async(&m_key[0], m_key.size(),
                              &m_blk[0], m_blk.size(),
                              *this, i_cp);
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
