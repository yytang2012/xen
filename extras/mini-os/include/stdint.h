#include <arch_wordsize.h>

/* From Mirage */

typedef signed char int8_t;
#if defined(__SHRT_MAX__) && (__SHRT_MAX__ + 0 != 32767)
typedef int int16_t __attribute__((__mode__(__HI__)));
#else
typedef signed short int16_t;
#endif
#if defined(__INT_MAX__) && (__INT_MAX__ + 0 != 2147483647)
typedef int int32_t __attribute__((__mode__(__SI__)));
#else
typedef signed int int32_t;
#endif

typedef unsigned char uint8_t;
#if defined(__SHRT_MAX__) && (__SHRT_MAX__ + 0 != 32767)
typedef unsigned int uint16_t __attribute__((__mode__(__HI__)));
#else
typedef unsigned short uint16_t;
#endif
#if defined(__INT_MAX__) && (__INT_MAX__ + 0 != 2147483647)
typedef unsigned int uint32_t __attribute__((__mode__(__SI__)));
#else
typedef unsigned int uint32_t;
#endif

#if __WORDSIZE == 64
typedef signed long int64_t;
typedef unsigned long uint64_t;
typedef signed long int intmax_t;
typedef unsigned long int uintmax_t;
#else
__extension__ typedef signed long long int64_t;
__extension__ typedef unsigned long long uint64_t;
__extension__ typedef signed long long int intmax_t;
__extension__ typedef unsigned long long int uintmax_t;
#endif
