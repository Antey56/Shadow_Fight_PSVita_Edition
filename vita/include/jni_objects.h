#ifndef SF_JNI_OBJECTS_H
#define SF_JNI_OBJECTS_H
#include <stdint.h>
#include "android_asset_adapter.h"
/* Internal single-thread object model, NOT JNI jobject/JNIEnv ABI.
   Handles belong to one environment; never exchange them between environments.
   Root must be trusted/immutable. Manager borrowed until environment destruction. */
typedef struct SfJniObjects SfJniObjects;
typedef uint32_t SfJniRef;
enum { SF_JNI_OK=0, SF_JNI_BAD_REF=1, SF_JNI_BAD_TYPE=2,
       SF_JNI_CAPACITY=3, SF_JNI_BAD_METHOD=4 };
SfJniObjects *sf_jni_objects_create(const char *,size_t);
void sf_jni_objects_destroy(SfJniObjects *);
SfJniRef sf_jni_context(SfJniObjects *);
SfJniRef sf_jni_new_ref(SfJniObjects *,SfJniRef,int global);
int sf_jni_delete_ref(SfJniObjects *,SfJniRef,int global);
void sf_jni_clear_locals(SfJniObjects *);
int sf_jni_exception(const SfJniObjects *);
void sf_jni_exception_clear(SfJniObjects *);
SfJniRef sf_jni_context_call(SfJniObjects *,SfJniRef,const char *,const char *);
AAssetManager *sf_jni_asset_manager(SfJniObjects *,SfJniRef);
#endif
