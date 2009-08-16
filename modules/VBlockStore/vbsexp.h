#ifndef VBS_vbsexp_h__
#define VBS_vbsexp_h__

#if defined(VBS_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define VBS_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define VBS_EXP __attribute__ ((visibility("default")))
#       else
#           define VBS_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define VBS_EXP _declspec(dllimport)
#   else
#       define VBS_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // VBS_vbsexp_h__
