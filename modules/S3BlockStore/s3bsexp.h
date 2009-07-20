#ifndef S3BS_s3bsexp_h__
#define S3BS_s3bsexp_h__

#if defined(S3BS_IMPL) || defined(STATIC_LINK)
#   if defined(WIN32)
#       define S3BS_EXP _declspec(dllexport)
#   else
#       if defined(GCC_HASCLASSVISIBILITY)
#           define S3BS_EXP __attribute__ ((visibility("default")))
#       else
#           define S3BS_EXP
#       endif
#   endif
#else
#   if defined(WIN32)
#       define S3BS_EXP _declspec(dllimport)
#   else
#       define S3BS_EXP
#   endif
#endif

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // S3BS_s3bsexp_h__
