#ifndef utp_Scoped_h__
#define utp_Scoped_h__

namespace utp {

template <typename T>
class Scoped
{
public:
    typedef void (*Del)(T p);

    Scoped(T const & i_nil, Del i_del)
        : m_val(i_nil)
        , m_nil(i_nil)
        , m_del(i_del)
    {}

    Scoped(T const & i_val, T const & i_nil, Del i_del)
        : m_val(i_val)
        , m_nil(i_nil)
        , m_del(i_del)
    {}


    ~Scoped()
    {
        if (m_val != m_nil)
            m_del(m_val);
    }

    inline Scoped & operator=(T const & i_val)
    {
        // Delete any pre-existing value.
        if (m_val != m_nil)
            m_del(m_val);

        m_val = i_val;
        return *this;
    }

    inline T const operator->() const { return m_val; }

    inline operator T&() { return m_val; }

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
