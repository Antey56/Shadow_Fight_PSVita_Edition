#ifndef SF_BIONIC_MUTEX_H
#define SF_BIONIC_MUTEX_H
#include <stdint.h>
#include "android_abi.h"
/* Android ARM32 inline storage, not a native pthread_mutex_t. */
SF_ANDROID_ABI int sf_bionic_mutexattr_init(int32_t *);
SF_ANDROID_ABI int sf_bionic_mutexattr_destroy(int32_t *);
SF_ANDROID_ABI int sf_bionic_mutexattr_settype(int32_t *,int);
SF_ANDROID_ABI int sf_bionic_mutex_init(int32_t *,const int32_t *);
SF_ANDROID_ABI int sf_bionic_mutex_destroy(int32_t *);
SF_ANDROID_ABI int sf_bionic_mutex_lock(int32_t *);
SF_ANDROID_ABI int sf_bionic_mutex_trylock(int32_t *);
SF_ANDROID_ABI int sf_bionic_mutex_unlock(int32_t *);
#endif
