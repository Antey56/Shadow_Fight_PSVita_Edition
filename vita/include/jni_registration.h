#ifndef SF_JNI_REGISTRATION_H
#define SF_JNI_REGISTRATION_H
#include <stdint.h>
#include "native_registry.h"
typedef struct {const char *name,*signature;void *fnPtr;} SfJniNativeMethod;
typedef struct {uintptr_t begin,end;} SfJniCodeRange;
typedef struct {char class_name[128],name[128],signature[512];uintptr_t address;} SfRegisteredNative;
typedef struct {SfRegisteredNative entries[128];size_t count;} SfNativeCatalog;
/* Trusted pointers and readable strings only. Borrowed policy, owned records.
   Validates ARM32 address/range/alignment; does not execute or map code.
   Zero success; -1 arguments, -2 conflict, -3 capacity, -4 declaration,
   -5 code range. On failure the entire catalog remains unchanged. */
int sf_jni_register_batch(SfNativeCatalog *,const NativeDeclaration *,size_t,
                          const SfJniCodeRange *,size_t,const char *,
                          const SfJniNativeMethod *,int32_t);
uintptr_t sf_jni_registered(const SfNativeCatalog *,const char *,const char *,const char *);
#endif
