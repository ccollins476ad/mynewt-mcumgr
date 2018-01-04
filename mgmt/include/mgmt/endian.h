#ifndef H_MGMT_ENDIAN_
#define H_MGMT_ENDIAN_

#include <inttypes.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#ifndef ntohs
#define ntohs(x) (x)
#endif

#ifndef htons
#define htons(x) (x)
#endif

#else
/* Little endian. */

#ifndef ntohs
#define ntohs(x)   ((uint16_t)  \
    ((((x) & 0xff00) >> 8) |    \
     (((x) & 0x00ff) << 8)))
#endif

#ifndef htons
#define htons(x) (ntohs(x))
#endif

#endif

#endif
