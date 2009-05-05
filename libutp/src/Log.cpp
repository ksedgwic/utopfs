#include <stdio.h>

#if !defined(WIN32)
#include <unistd.h>
#include <pthread.h>
#endif

#if defined(WIN32)
#include <winsock2.h>
#include <windows.h>
#endif

#include <algorithm>
#include <string>

#include <ace/Synch.h>
#include <ace/Atomic_Op.h>

#include "Log.h"

using namespace std;
using namespace utp;

// ----------------------------------------------------------------
// Assembly Class
// ----------------------------------------------------------------
//
// This class is used to safely assemble the category tree as the
// categories are constructed in random order.

namespace
{
// This class implements a serially incrementing thread tag
// (ordinal-id) which can be used by the ACE_TSS template ...
class ThreadTag
{
public:
    ThreadTag() : m_tagval(++g_nexttag) {}
    ~ThreadTag() {}
    int getValue() const { return m_tagval; }
private:
    static ACE_Atomic_Op<ACE_Thread_Mutex, long> g_nexttag;
    int				m_tagval;
};

ACE_Atomic_Op<ACE_Thread_Mutex, long> ThreadTag::g_nexttag(0);

ACE_TSS<ThreadTag>	tss_threadtag;

class Assembly
{
public:
    static Assembly * instance()
    {
        static Assembly * assembly = NULL;
        if (!assembly)
            assembly = new Assembly();
        return assembly;
    }

    Assembly() : m_catseq_mutex() {}

    void add(LogCategory * logcat)
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_catseq_mutex);

        // mix it up ...
        LogCategorySeq::iterator pos;
        for (pos = m_catseq.begin(); pos != m_catseq.end(); pos++) {
            if (logcat == (*pos)->parent())
                logcat->subcategory_add(*pos);
            if ((*pos) == logcat->parent())
                (*pos)->subcategory_add(logcat);
        }

        // we go on the end ...
        m_catseq.push_back(logcat);
    }

    void remove(LogCategory * logcat)
    {
        ACE_Guard<ACE_Thread_Mutex> guard(m_catseq_mutex);

        // remove this category from it's parent *if* it's
        // parent is still alive ...
        LogCategorySeq::iterator pos;
        for (pos = m_catseq.begin(); pos != m_catseq.end(); pos++)
            if ((*pos) == logcat->parent())
                (*pos)->subcategory_rem(logcat);

        // don't worry about the kids ... they'll get "removed"
        // automatically when the category goes away.

        // remove this category from the assmebly
        m_catseq.erase(std::remove(m_catseq.begin(),
                                   m_catseq.end(),
                                   logcat),
                       m_catseq.end());
    }

    mutable ACE_Thread_Mutex	m_catseq_mutex;
    LogCategorySeq				m_catseq;
};

LogFieldHandle tstamp()
{
    return new TimeLogField("tstamp", T64::now());
}

LogFieldHandle threadid()
{
    // Use an ordinal tag, the linux threads are too ugly ...
    return new IntegerLogField("threadid", tss_threadtag->getValue());
#if 0
#if defined(WIN32)
    return new IntegerLogField("threadid",
                               static_cast<int>(GetCurrentThreadId()));
#else
    return new IntegerLogField("threadid",
                               static_cast<int>(pthread_self()));
#endif
#endif
}

LogFieldHandle pid()
{
    return new IntegerLogField("pid", static_cast<int>(getpid()));
}

} // end namespace

namespace utp {

LogCategory theRootLogCategory("ROOT");

LogCategory::LogCategory(string const & name,
                         LogCategory * parent)
    : m_name(name)
    , m_parent(parent)
    , m_level(0)
    , m_logcat_mutex()
{
    Assembly::instance()->add(this);
}

LogCategory::~LogCategory()
{
    Assembly::instance()->remove(this);
}

bool
LogCategory::is_enabled(int level) const
{
    // test the category level first, without the lock
    if (m_level < level)
        return false;

    ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_logcat_mutex);

    // if any logger is interested, we are enabled
    LoggerSeq::const_iterator pos;
    for (pos = m_loggers.begin(); pos != m_loggers.end(); pos++)
        if ((*pos)->is_enabled(level))
            return true;

    return false;
}

void
LogCategory::level(int level, bool recursive)
{
    m_level = level;

    if (recursive) {
        ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_logcat_mutex);
        LogCategorySeq::iterator pos;
        for (pos = m_subcat.begin(); pos != m_subcat.end(); pos++)
            (*pos)->level(level, recursive);
    }
}

void
LogCategory::debug_str(int level, const LogFieldSeq & valseq)
{
    if (m_level < level)
        return;

    // copy the value sequence, prepend our category name
    LogFieldSeq catvalseq;
    catvalseq.reserve(6);
    catvalseq.push_back(tstamp());
    catvalseq.push_back(threadid());
    catvalseq.push_back(new StringLogField("category", m_name));
    catvalseq.insert(catvalseq.end(), valseq.begin(), valseq.end());

    ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_logcat_mutex);

    // all of the loggers get called ...
    LoggerSeq::iterator pos;
    for (pos = m_loggers.begin(); pos != m_loggers.end(); pos++)
        (*pos)->debug_str(level, catvalseq);
}

void
LogCategory::logger_add(Logger * logger, bool recursive)
{
    ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_logcat_mutex);

    // add to our own list
    m_loggers.push_back(logger);

    // optionally recurse ...
    if (recursive) {
        LogCategorySeq::iterator pos;
        for (pos = m_subcat.begin(); pos != m_subcat.end(); pos++)
            (*pos)->logger_add(logger, recursive);
    }
}

void
LogCategory::logger_rem(Logger * logger, bool recursive)
{
    ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_logcat_mutex);

    // optionally recurse ...
    if (recursive) {
        LogCategorySeq::iterator pos;
        for (pos = m_subcat.begin(); pos != m_subcat.end(); pos++)
            (*pos)->logger_rem(logger, recursive);
    }

    // remove from our own list
    m_loggers.erase(remove(m_loggers.begin(), m_loggers.end(), logger),
                    m_loggers.end());
}

LogCategory *
LogCategory::parent() const
{
    return m_parent;
}

void
LogCategory::subcategories(LogCategorySeq & outseq) const
{
    ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_logcat_mutex);
    outseq = m_subcat;
}

void
LogCategory::subcategory_add(LogCategory * subcat)
{
    ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_logcat_mutex);
    m_subcat.push_back(subcat);

    // kid (and descendents) inherit our loggers
    LoggerSeq::iterator pos;
    for (pos = m_loggers.begin(); pos != m_loggers.end(); pos++)
        subcat->logger_add(*pos, true);

    // kid (and descendents) inherit our level
    subcat->level(m_level, true);
}

void
LogCategory::subcategory_rem(LogCategory * subcat)
{
    ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_logcat_mutex);

    // remove the subcategory ...
    m_subcat.erase(remove(m_subcat.begin(), m_subcat.end(), subcat),
                   m_subcat.end());
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

