#include <iostream>

#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "vbslog.h"
#include "VBSRefreshBlockRequest.h"
#include "VBSGetRequest.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSRefreshBlockRequest::VBSRefreshBlockRequest(VBlockStore & i_vbs,
                                               long i_outstanding,
                                               uint64 i_rid,
                                               void const * i_keydata,
                                               size_t i_keysize,
                                               RefreshBlockCompletion & i_cmpl,
                                               void const * i_argp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_rid(i_rid)
    , m_key((uint8 const *) i_keydata, (uint8 const *) i_keydata + i_keysize)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
{
    LOG(lgr, 6, "RFRSH @" << (void *) this << " CTOR");
}

VBSRefreshBlockRequest::~VBSRefreshBlockRequest()
{
    LOG(lgr, 6, "RFRSH @" << (void *) this << " DTOR");
}

void
VBSRefreshBlockRequest::stream_insert(std::ostream & ostrm) const
{
    ostrm << "RFRSH @" << (void *) this;
}

void
VBSRefreshBlockRequest::process(VBSChild * i_cp,
                                BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " process");

    i_bsh->bs_refresh_block_async(m_rid, &m_key[0], m_key.size(), *this, i_cp);
}

void
VBSRefreshBlockRequest::rb_complete(void const * i_keydata,
                                    size_t i_keysize,
                                    void const * i_argp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " rb_complete");

    bool do_complete = false;
    bool do_done = false;
    VBSChildSeq needy;

    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the first successful completion?
        if (!m_succeeded)
        {
            do_complete = true;
            m_succeeded = true;
            m_hadit = cp;
            needy.swap(m_needy);
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
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL GOOD");
        m_cmpl.rb_complete(i_keydata, i_keysize, m_argp);
    }

    // If there were any needy children, setup a put request for them.
    if (!needy.empty())
    {
        // Other blockstores are missing the block.  We heal this
        // situation by enquing a get request to ourself for the block
        // with the other blockstores pre-filled in as needy.

        VBSGetRequestHandle grh = new VBSGetRequest(m_vbs,
                                                    1,
                                                    i_keydata,
                                                    i_keysize,
                                                    NULL,
                                                    32 * 1024,
                                                    NULL,
                                                    NULL);

        for (unsigned ii = 0; ii < needy.size(); ++ii)
            grh->needy(needy[ii]);

        m_vbs.insert_request(grh);

        cp->enqueue_get(grh);
    }

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    if (do_done)
    {
        LOG(lgr, 6, *this << ' ' << "DONE");
        done();
    }
}

void
VBSRefreshBlockRequest::rb_missing(void const * i_keydata,
                                   size_t i_keysize,
                                   void const * i_argp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " rb_missing");

    bool do_complete = false;
    bool do_done = false;
    bool do_xcopy = false;
    VBSChildHandle hadit = NULL;

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

        // We're needy, did someone else succeed?
        if (m_succeeded)
        {
            // Yes, setup a put of the data to us below.
            hadit = m_hadit;
            do_xcopy = true;
        }
        else
        {
            // Nope, just add ourselves to the needy list.
            m_needy.push_back(cp);
        }
    }

    // If we are the last child back with an exception we
    // get to tell the parent ...
    //
    if (do_complete)
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL MISSING");
        m_cmpl.rb_missing(i_keydata, i_keysize, m_argp);
    }

    // If we were needy and someone else found data make a cross-copy
    // of the data.
    if (do_xcopy)
    {
        // Another blockstore had the block.  We heal our lack my
        // enqueing a get request on the blockstore that had it and
        // pre-filling our blockstore in as needy.
        //
        VBSGetRequestHandle grh = new VBSGetRequest(m_vbs,
                                                    1,
                                                    i_keydata,
                                                    i_keysize,
                                                    NULL,
                                                    32 * 1024,
                                                    NULL,
                                                    NULL);

        grh->needy(cp);

        m_vbs.insert_request(grh);

        hadit->enqueue_get(grh);
    }

    // This likely results in our destruction, do it last and
    // don't touch anything afterwards!
    //
    if (do_done)
    {
        LOG(lgr, 6, *this << ' ' << "DONE");
        done();
    }
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
