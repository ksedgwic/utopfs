#ifndef utp_Types_h__
#define utp_Types_h__

#include <vector>
#include <string>

namespace utp {

typedef unsigned char		uint8;
typedef char				int8;
typedef unsigned short		uint16;
typedef short				int16;
typedef unsigned long		uint32;
typedef long				int32;
typedef signed long long	int64;
typedef unsigned long long	uint64;

typedef std::vector<uint8> OctetSeq;

typedef std::vector<std::string> StringSeq;

} // end namespace utp

// Local Variables:
// mode: C++
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets: ((comment-intro . 0))
// End:

#endif // utp_Types_h__
