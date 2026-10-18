#ifndef SF_BIONIC_ONCE_H
#define SF_BIONIC_ONCE_H
#include <stdint.h>
#include "android_abi.h"
/* Android ARM32 pthread_once_t storage, initially zero, 4-byte aligned.
   No fork/cancellation/initializer-unwind support is supplied by this port. */
typedef void (SF_ANDROID_ABI *SfOnceInit)(void);
SF_ANDROID_ABI int sf_bionic_pthread_once(volatile int32_t *,SfOnceInit);
#endif
