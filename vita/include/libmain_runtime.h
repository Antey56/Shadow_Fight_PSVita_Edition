#ifndef SF_LIBMAIN_RUNTIME_H
#define SF_LIBMAIN_RUNTIME_H
#include "android_abi.h"
#include "arm_scope.h"
#include "arm_namespace.h"
#include <stddef.h>
#include <stdint.h>
typedef void (SF_ANDROID_ABI *SfDtor)(void *);
SF_ANDROID_ABI int sf_runtime_atexit(SfDtor,void *,void *);
SF_ANDROID_ABI void sf_runtime_finalize(void *);
int sf_libmain_runtime_catalog(ArmScopeEntry *,size_t);
int sf_libmain_runtime_prepare(void);
void sf_libmain_runtime_namespace(ArmNamespace *);
void sf_libmain_runtime_bind(uint32_t,size_t,uint32_t,uint32_t);
#endif
