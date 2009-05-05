#ifndef utp_RC_h__
#define utp_RC_h__

/// @file RC.h
/// Reference Counting Template.

// This reference counting design is based on Section 29 of "More
// Effective C++" by Scott Meyers.

#include <ace/Synch.h>
#include <ace/Atomic_Op.h>

#include "utpexp.h"

namespace utp {

class UTP_EXP RCObj
{
public:
    /// Default constructor.
    RCObj() : m_refcount(0) {}

    /// Copy constructor.
    RCObj(RCObj const &) : m_refcount(0) {}

    /// Destructor.
    virtual ~RCObj() {}

    /// Assignment operator.
    RCObj& operator=(RCObj const &) { return *this; }

    /// Add a reference, returns refcount after increment.
	virtual long rc_add_ref(void * ptr = NULL) const
    {
        return ++m_refcount;
    }

    /// Remove a reference, returns refcount after decrement.
	virtual long rc_rem_ref(void * ptr = NULL) const
    {
        long val = --m_refcount;
        if (val == 0)
            delete this;
        return val;
    }

    /// Returns reference count.
    long rc_count() const
    {
        return m_refcount.value();
    }

private:
    mutable ACE_Atomic_Op<ACE_Thread_Mutex, long>	m_refcount;
};

template <typename T>
class RCPtr
{
public:
    inline RCPtr(T* ptr = 0)
        : m_ptr(ptr)
    {
        initialize();
    }

    inline RCPtr(RCPtr const & rhs)
        : m_ptr(rhs.m_ptr)
    {
        initialize();
    }

    template <typename T2>
    friend class RCPtr;

    /// Converting Constructor
    template <typename S>
    RCPtr(RCPtr<S> const & cp)
        : m_ptr(cp.m_ptr)
    {
        initialize();
    }

    // Desctructor
    inline ~RCPtr()
    {
        terminate();
    }

    /// Assignment operator
    inline RCPtr& operator=(RCPtr const & rhs)
    {
        if (m_ptr != rhs.m_ptr)
        {
            terminate();
            m_ptr = rhs.m_ptr;
            initialize();
        }

        return *this;
    }

    /// Dereference (for member) operator, returns pointer.
    inline T* operator->() const
    {
        return m_ptr;
    }

    /// Dereference operator, returns reference.
    inline T& operator*() const
    {
        return *m_ptr;
    }

    /// Null predicate.
    inline bool operator!() const { return m_ptr == 0; }

    /// Non-Null predicate.
    inline operator bool() const { return m_ptr != 0; }

    /// Less-than predicate.
    inline bool operator<(RCPtr const & b) const
    {
        return *m_ptr < *b.m_ptr;
    }

    /// Less-than-or-equal predicate.
    inline bool operator<=(RCPtr const & b) const
    {
        return *m_ptr <= *b.m_ptr;
    }

    /// Greater-than predicate.
    inline bool operator>(RCPtr const & b) const
    {
        return *m_ptr > *b.m_ptr;
    }

    /// Greater-than-or-equal predicate.
    inline bool operator>=(RCPtr const & b) const
    {
        return *m_ptr >= *b.m_ptr;
    }

    /// Equals predicate.
    inline bool operator==(RCPtr const & b) const
    {
        return *m_ptr == *b.m_ptr;
    }

    /// Not-equals predicate.
    inline bool operator!=(RCPtr const & b) const
    {
        return *m_ptr != *b.m_ptr;
    }

    /// Strict equality predicate.
    inline bool same(RCPtr const & other) const
    {
        return m_ptr == other.m_ptr;
    }

private:
    inline void initialize()
    {
        if (m_ptr)
            m_ptr->rc_add_ref((void *) &m_ptr);
    }

    inline void terminate()
    {
        if (m_ptr)
            m_ptr->rc_rem_ref((void *) &m_ptr);
    }

    T *		m_ptr;
};

/// Delegate stream insertion to the referenced object.
template<typename T>
std::ostream & operator<<(std::ostream & s, RCPtr<T> const & p)
{
    s << *p.m_ptr;
    return s;
}

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_RC_h__
