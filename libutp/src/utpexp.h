#ifndef utp_utpexp_h__
#define utp_utpexp_h__

#if defined(UTP_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define UTP_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define UTP_EXP __attribute__ ((visibility("default")))
#       else
#           define UTP_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define UTP_EXP _declspec(dllimport)
#   else
#       define UTP_EXP
#   endif
#endif

// With G++ exceptions must always be exported in *all* modules
// See http://gcc.gnu.org/wiki/Visibility for more info.
//
#if defined(WIN32)
#   define UTP_EXC_EXP UTP_EXP
#else
#   if defined(GCC_HASCLASSVISIBILITY)
#      define UTP_EXC_EXP __attribute__ ((visibility("default")))
#   else
#      define UTP_EXC_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_utpexp_h__
