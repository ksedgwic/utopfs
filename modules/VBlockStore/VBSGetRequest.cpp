#include <iostream>

#include "Log.h"

#include "VBlockStore.h"
#include "VBSChild.h"
#include "VBSGetRequest.h"
#include "VBSPutRequest.h"
#include "vbslog.h"
#include "VBSRequest.h"

using namespace std;
using namespace utp;

namespace VBS {

VBSGetRequest::VBSGetRequest(VBlockStore & i_vbs,
                             long i_outstanding,
                             void const * i_keydata,
                             size_t i_keysize,
                             void * o_buffdata,
                             size_t i_buffsize,
                             BlockStore::BlockGetCompletion * i_cmpl,
                             void const * i_argp)
    : VBSRequest(i_vbs, i_outstanding)
    , m_key((uint8 const *) i_keydata, (uint8 const *) i_keydata + i_keysize)
    , m_buffdata(o_buffdata)
    , m_buffsize(i_buffsize)
    , m_cmpl(i_cmpl)
    , m_argp(i_argp)
{
    LOG(lgr, 6, "GET @" << (void *) this << ' ' << keystr(m_key) << " CTOR");
}

VBSGetRequest::~VBSGetRequest()
{
    LOG(lgr, 6, "GET @" << (void *) this << ' ' << keystr(m_key) << " DTOR");
}

void
VBSGetRequest::stream_insert(std::ostream & ostrm) const
{
    ostrm << "GET @" << (void *) this << ' ' << keystr(m_key);
}

void
VBSGetRequest::initiate(VBSChild * i_cp, BlockStoreHandle const & i_bsh)
{
    LOG(lgr, 6, *this << " initiate " << i_cp->instname());

    {
        // Allocate our buffer now.
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);
        m_blk.resize(m_buffsize);
    }

    i_bsh->bs_block_get_async(&m_key[0], m_key.size(),
                              &m_blk[0], m_blk.size(),
                              *this, i_cp);
}

void
VBSGetRequest::bg_complete(void const * i_keydata,
                           size_t i_keysize,
                           void const * i_argp,
                           size_t i_blksize)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " bg_complete");

    cp->report_get(i_blksize);

    bool do_complete = false;
    bool do_done = false;
    VBSChildSeq needy;
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_vbsreqmutex);

        // Are we the first successful completion?
        if (!m_succeeded)
        {
            // Make sure the buffer size was OK.
            if (i_blksize > m_blk.size())
                throwstream(InternalError, FILELINE
                            << "unexpected return of " << i_blksize
                            << " bytes into buffer of size " << m_blk.size());

            m_retsize = i_blksize;

            do_complete = true;
            m_succeeded = true;
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
    if (do_complete && m_cmpl)
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL GOOD");

        // Copy the data into the parent buffer.
        if (m_buffdata)
            ACE_OS::memcpy(m_buffdata, &m_blk[0], i_blksize);

        m_cmpl->bg_complete(&m_key[0], m_key.size(), m_argp, i_blksize);

        // Cancel any other chilren's requests.
        m_vbs.cancel_get(cp, m_key);
    }

    // If there were any needy children, setup a put request for them.
    if (!needy.empty())
    {
        for (unsigned ii = 0; ii < needy.size(); ++ii)
        {
            LOG(lgr, 6, *this << ' '
                << "NEEDY CHILD: " << needy[ii]->instname());

        }

        VBSPutRequestHandle prh = new VBSPutRequest(m_vbs,
                                                    needy.size(),
                                                    &m_key[0],
                                                    m_key.size(),
                                                    &m_blk[0],
                                                    m_retsize,
                                                    NULL,
                                                    NULL);

        m_vbs.insert_req(prh);

        for (unsigned ii = 0; ii < needy.size(); ++ii)
            needy[ii]->enqueue_put(prh);
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
VBSGetRequest::bg_error(void const * i_keydata,
                        size_t i_keysize,
                        void const * i_argp,
                        Exception const & i_exp)
{
    VBSChild * cp = (VBSChild *) i_argp;

    LOG(lgr, 6, *this << ' ' << cp->instname() << " bg_error");

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
    if (do_complete && m_cmpl)
    {
        LOG(lgr, 6, *this << ' ' << "UPCALL ERROR");
        m_cmpl->bg_error(&m_key[0], m_key.size(), m_argp, i_exp);
    }

    if (i_exp.type() == Exception::T_NOTFOUND)
    {
        // If we didn't find the block add it to our need list.
        //
        // Only do this if the error was NotFound, otherwise
        // we were canceled and we don't need to do this ...
        //
        cp->needed_keys_append(&m_key[0], m_key.size());
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
VBSGetRequest::needy(VBSChildHandle const & i_needy)
{
    m_needy.push_back(i_needy);
}

} // namespace VBS

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:
