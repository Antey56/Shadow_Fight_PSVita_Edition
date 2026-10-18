#ifndef SF_AEABI_MEMORY_H
#define SF_AEABI_MEMORY_H
#include "android_abi.h"
#include "arm_scope.h"
#include <stddef.h>
#define SF_AEABI_VARIANTS(X) X(memcpy) X(memcpy4) X(memcpy8) X(memmove) X(memmove4) X(memmove8)
#define SF_DECLARE_COPY(n) SF_ANDROID_ABI void sf_aeabi_##n(void *,const void *,size_t);
SF_AEABI_VARIANTS(SF_DECLARE_COPY)
#undef SF_DECLARE_COPY
SF_ANDROID_ABI void sf_aeabi_memset(void *,size_t,int);
SF_ANDROID_ABI void sf_aeabi_memset4(void *,size_t,int);
SF_ANDROID_ABI void sf_aeabi_memset8(void *,size_t,int);
SF_ANDROID_ABI void sf_aeabi_memclr(void *,size_t);
SF_ANDROID_ABI void sf_aeabi_memclr4(void *,size_t);
SF_ANDROID_ABI void sf_aeabi_memclr8(void *,size_t);
int sf_aeabi_memory_catalog(ArmScopeEntry *,size_t);
#endif
