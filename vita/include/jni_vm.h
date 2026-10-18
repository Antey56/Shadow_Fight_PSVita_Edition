#ifndef SF_JNI_VM_H
#define SF_JNI_VM_H
#include "jni_bridge.h"
enum {SF_JNI_ERR=-1,SF_JNI_EDETACHED=-2,SF_JNI_EVERSION=-3};
typedef struct {int32_t version;char *name;SfJObject group;} SfJniAttachArgs;
typedef struct SfJavaVMTable {
    void *reserved[3];
    int32_t (SF_ANDROID_ABI *DestroyJavaVM)(SfJavaVM *);
    int32_t (SF_ANDROID_ABI *AttachCurrentThread)(SfJavaVM *,void **,void *);
    int32_t (SF_ANDROID_ABI *DetachCurrentThread)(SfJavaVM *);
    int32_t (SF_ANDROID_ABI *GetEnv)(SfJavaVM *,void **,int32_t);
    int32_t (SF_ANDROID_ABI *AttachCurrentThreadAsDaemon)(SfJavaVM *,void **,void *);
} SfJavaVMTable;
/* Restricted owner-thread VM. Worker attachment explicitly unsupported.
   Create attached; DestroyJavaVM marks closed but keeps storage so a second
   destroy returns an error. Release only on owner after joining all workers. */
SfJavaVM *sf_jni_vm_create(const char *,size_t);
int sf_jni_vm_release(SfJavaVM *);
#endif
