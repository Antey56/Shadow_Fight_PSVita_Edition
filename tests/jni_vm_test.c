#include "jni_vm.h"
#include <assert.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
typedef struct {SfJavaVM *vm;SfJniEnv *env;} Work;
static void worker(Work *w){
    void *out=(void *)1;
    assert((*w->vm)->GetEnv(w->vm,&out,0x10006)==SF_JNI_EDETACHED);assert(!out);
    out=(void *)1;assert((*w->vm)->AttachCurrentThread(w->vm,&out,0)==SF_JNI_ERR);assert(!out);
    assert((*w->vm)->DetachCurrentThread(w->vm)==0);
    assert((*w->env)->GetVersion(w->env)==0); /* Wrong-thread env rejected. */
    assert(!sf_jni_bridge_context(w->env));
    assert(sf_jni_vm_release(w->vm)==SF_JNI_ERR);
}
#ifdef _WIN32
static DWORD WINAPI worker_entry(LPVOID p){worker(p);return 0;}
#else
static void *worker_entry(void *p){worker(p);return 0;}
#endif
int main(void){
    SfJavaVM *vm=sf_jni_vm_create(".",4096);assert(vm);
    SfJniEnv *env=0;assert((*vm)->GetEnv(vm,(void **)&env,0x10006)==0);assert(env);
    SfJavaVM *back=0;assert((*env)->GetJavaVM(env,&back)==0&&back==vm);
    void *out=(void *)1;assert((*vm)->GetEnv(vm,&out,0x10008)==SF_JNI_EVERSION&&!out);
    assert((*vm)->GetEnv(vm,0,0x10006)==SF_JNI_ERR);
    Work w={vm,env};
#ifdef _WIN32
    HANDLE t=CreateThread(0,0,worker_entry,&w,0,0);assert(t);
    assert(WaitForSingleObject(t,INFINITE)==WAIT_OBJECT_0);CloseHandle(t);
#else
    pthread_t t;assert(!pthread_create(&t,0,worker_entry,&w));assert(!pthread_join(t,0));
#endif
    SfJObject local=sf_jni_bridge_context(env);assert(local);
    SfJObject global=(*env)->NewGlobalRef(env,local);assert(global);
    SfJObject cls=(*env)->GetObjectClass(env,global);
    SfJMethod m=(*env)->GetMethodID(env,cls,"getAssets","()Landroid/content/res/AssetManager;");assert(m);
    assert(!(*env)->FindClass(env,"unknown/Class"));
    SfJObject error=(*env)->ExceptionOccurred(env);assert(error);
    (*env)->ExceptionClear(env);
    SfJObject saved_error=(*env)->NewGlobalRef(env,error);assert(saved_error);
    out=0;assert((*vm)->AttachCurrentThread(vm,&out,0)==0&&out==env);
    assert((*vm)->DetachCurrentThread(vm)==0);assert((*vm)->DetachCurrentThread(vm)==0);
    out=(void *)1;assert((*vm)->GetEnv(vm,&out,0x10006)==SF_JNI_EDETACHED&&!out);
    assert((*env)->GetVersion(env)==0);
    SfJniAttachArgs args={0x10008,0,0};
    assert((*vm)->AttachCurrentThread(vm,&out,&args)==SF_JNI_EVERSION&&!out);
    args.version=0x10006;args.name="unsupported";
    assert((*vm)->AttachCurrentThread(vm,&out,&args)==SF_JNI_ERR&&!out);
    args.name=0;assert((*vm)->AttachCurrentThreadAsDaemon(vm,&out,&args)==0&&out==env);
    assert((*env)->Throw(env,saved_error)==0);assert((*env)->ExceptionCheck(env));
    (*env)->ExceptionClear(env);(*env)->DeleteGlobalRef(env,saved_error);
    assert((*env)->CallObjectMethodA(env,global,m,0));
    assert(!(*env)->CallObjectMethodA(env,local,m,0));assert((*env)->ExceptionCheck(env));
    (*env)->ExceptionClear(env);(*env)->DeleteGlobalRef(env,global);
    assert((*vm)->DestroyJavaVM(vm)==0);
    assert((*vm)->DestroyJavaVM(vm)==SF_JNI_ERR);
    out=(void *)1;assert((*vm)->GetEnv(vm,&out,0x10006)==SF_JNI_ERR&&!out);
    assert(sf_jni_vm_release(vm)==0);puts("owner-thread JavaVM PASS");return 0;
}
