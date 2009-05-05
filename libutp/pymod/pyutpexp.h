#ifndef pyutpexp_h__
#define pyutpexp_h__

#if defined(PYUTP_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define PYUTP_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define PYUTP_EXP __attribute__ ((visibility("default")))
#       else
#           define PYUTP_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define PYUTP_EXP _declspec(dllimport)
#   else
#       define PYUTP_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // pyutpexp_h__
