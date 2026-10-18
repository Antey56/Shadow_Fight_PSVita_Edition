#include "jni_vm.h"
#include <stddef.h>
#include <stdlib.h>
#if defined(SF2_VITA_TARGET)
#include <psp2/kernel/threadmgr.h>
typedef SceUID OwnerThread;
static OwnerThread thread_id(void){return sceKernelGetThreadId();}
static int same_thread(OwnerThread t){return t==thread_id();}
#elif defined(_WIN32)
#include <windows.h>
typedef DWORD OwnerThread;
static OwnerThread thread_id(void){return GetCurrentThreadId();}
static int same_thread(OwnerThread t){return t==thread_id();}
#else
#include <pthread.h>
typedef pthread_t OwnerThread;
static OwnerThread thread_id(void){return pthread_self();}
static int same_thread(OwnerThread t){return pthread_equal(t,thread_id());}
#endif
typedef struct {SfJavaVM table;OwnerThread owner;SfJniEnv *env;int attached,closed;} VM;
static const SfJavaVMTable table;
#define CHECK_SLOT(field,n) typedef char check_##field[offsetof(SfJavaVMTable,field)==(n)*sizeof(SfJniUnused)?1:-1]
CHECK_SLOT(DestroyJavaVM,3);CHECK_SLOT(AttachCurrentThread,4);CHECK_SLOT(DetachCurrentThread,5);
CHECK_SLOT(GetEnv,6);CHECK_SLOT(AttachCurrentThreadAsDaemon,7);
static VM *state(SfJavaVM *vm){return vm&&*vm==&table?(VM *)vm:0;}
static int allowed(void *context){
    VM *s=context;return same_thread(s->owner)&&!s->closed&&s->attached;
}
static int version_supported(int32_t v){return v==0x10001||v==0x10002||v==0x10004||v==0x10006;}
static SF_ANDROID_ABI int32_t get_env(SfJavaVM *vm,void **out,int32_t version){
    if(!out)return SF_JNI_ERR;
    *out=0;VM *s=state(vm);if(!s)return SF_JNI_ERR;
    if(!version_supported(version))return SF_JNI_EVERSION;
    if(!same_thread(s->owner))return SF_JNI_EDETACHED;
    if(s->closed)return SF_JNI_ERR;
    if(!s->attached)return SF_JNI_EDETACHED;
    *out=s->env;return 0;
}
static SF_ANDROID_ABI int32_t attach(SfJavaVM *vm,void **out,void *arguments){
    if(!out)return SF_JNI_ERR;
    *out=0;VM *s=state(vm);
    if(!s||!same_thread(s->owner)||s->closed)return SF_JNI_ERR;
    /* Already attached: preserve env; JNI ignores attach arguments here. */
    if(s->attached){*out=s->env;return 0;}
    if(arguments){
        SfJniAttachArgs *args=arguments;
        if(!version_supported(args->version))return SF_JNI_EVERSION;
        if(args->name||args->group)return SF_JNI_ERR; /* No Java Thread objects. */
    }
    s->attached=1;*out=s->env;return 0;
}
static SF_ANDROID_ABI int32_t detach(SfJavaVM *vm){
    VM *s=state(vm);if(!s)return SF_JNI_ERR;
    if(!same_thread(s->owner))return 0; /* Unattached worker: no-op. */
    if(s->closed)return SF_JNI_ERR;
    if(s->attached){
        sf_jni_bridge_clear_locals(s->env);
        (*s->env)->ExceptionClear(s->env);s->attached=0;
    }
    return 0;
}
static SF_ANDROID_ABI int32_t destroy(SfJavaVM *vm){
    VM *s=state(vm);if(!s||!same_thread(s->owner)||s->closed)return SF_JNI_ERR;
    /* No Java execution/monitors exist. Global handles cease at destruction. */
    s->attached=1;sf_jni_bridge_destroy(s->env);s->env=0;s->attached=0;s->closed=1;return 0;
}
static const SfJavaVMTable table={.DestroyJavaVM=destroy,.AttachCurrentThread=attach,
    .DetachCurrentThread=detach,.GetEnv=get_env,.AttachCurrentThreadAsDaemon=attach};
SfJavaVM *sf_jni_vm_create(const char *root,size_t limit){
    VM *s=calloc(1,sizeof(*s));if(!s)return 0;
    s->env=sf_jni_bridge_create(root,limit);if(!s->env){free(s);return 0;}
    s->table=&table;s->owner=thread_id();s->attached=1;
    sf_jni_bridge_bind_vm(s->env,&s->table,allowed,s);return &s->table;
}
int sf_jni_vm_release(SfJavaVM *vm){
    VM *s=state(vm);if(!s||!same_thread(s->owner))return SF_JNI_ERR;
    if(!s->closed)destroy(vm);
    free(s);return 0;
}
