#ifndef utp_Scoped_h__
#define utp_Scoped_h__

namespace utp {

template <typename T>
class Scoped
{
public:
    /// A deletion function.
    typedef void (*Del)(T p);

    /// Default constructor.
    ///
    /// Note - the deletion function will not be called on the nil
    /// value.
    ///
    /// @param[in] i_nil Nil value.
    /// @param[in] i_del Deletion functor.
    ///
    Scoped(T const & i_nil, Del i_del)
        : m_val(i_nil)
        , m_nil(i_nil)
        , m_del(i_del)
    {}

    /// Contructor from value.
    ///
    /// Note - the deletion function will not be called on the nil
    /// value.
    ///
    /// @param[in] i_val The value to assign.
    /// @param[in] i_nil Nil value.
    /// @param[in] i_del Deletion functor.
    ///
    Scoped(T const & i_val, T const & i_nil, Del i_del)
        : m_val(i_val)
        , m_nil(i_nil)
        , m_del(i_del)
    {}


    /// Destructor, calls deletion function on non-nil values.
    ///
    ~Scoped()
    {
        if (m_val != m_nil)
            m_del(m_val);
    }

    /// Assignment operator.
    ///
    /// Calls deletion on existing non-nil value and assigns new
    /// value.
    ///
    /// @param[in] i_val The right-hand-side is the new value.
    ///
    inline Scoped & operator=(T const & i_val)
    {
        // Delete any pre-existing value.
        if (m_val != m_nil)
            m_del(m_val);

        m_val = i_val;
        return *this;
    }

    /// Pointer dereference.
    ///
    inline T const operator->() const { return m_val; }

    /// Reference.
    ///
    inline operator T&() { return m_val; }

    /// Takes value, will not be deleted.
    ///
    T const take()
    {
        T tmp = m_val;
        m_val = m_nil;
        return tmp;
    }

private:
    T			m_val;
    T			m_nil;
    Del			m_del;
};

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Scoped_h__
