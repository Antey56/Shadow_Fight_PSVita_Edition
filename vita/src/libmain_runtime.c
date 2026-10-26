/* Bounded services for the hash-pinned libmain JNI_OnLoad probe.
 * This is not a complete Android libc, dynamic linker or exception runtime. */
#include "libmain_runtime.h"
#include "bionic_once.h"
#include "preloaded_dl.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef SF2_VITA_TARGET
#include <psp2/kernel/rng.h>
#include <psp2/kernel/threadmgr.h>
#endif
static struct {SfDtor function;void *argument,*dso;uint64_t order;} destructors[64];
static uint64_t serial;
static unsigned lock;
static void acquire(void){while(__atomic_exchange_n(&lock,1,__ATOMIC_ACQUIRE)){
#ifdef SF2_VITA_TARGET
    sceKernelDelayThread(1000);
#endif
}}
static void unlock(void){__atomic_store_n(&lock,0,__ATOMIC_RELEASE);}
SF_ANDROID_ABI int sf_runtime_atexit(SfDtor function,void *argument,void *dso){
    if(!function)return -1;
    acquire();
    if(serial==UINT64_MAX){unlock();return -1;}
    for(size_t i=0;i<64;i++)if(!destructors[i].function){
        destructors[i].function=function;destructors[i].argument=argument;
        destructors[i].dso=dso;destructors[i].order=++serial;unlock();return 0;
    }
    unlock();return -1;
}
SF_ANDROID_ABI void sf_runtime_finalize(void *dso){
    for(;;){
        acquire();size_t chosen=64;uint64_t last=0;
        for(size_t i=0;i<64;i++)if(destructors[i].function&&(!dso||destructors[i].dso==dso)&&destructors[i].order>last){chosen=i;last=destructors[i].order;}
        if(chosen==64){unlock();return;}
        SfDtor callback=destructors[chosen].function;void *argument=destructors[chosen].argument;
        destructors[chosen].function=0;unlock();callback(argument);
    }
}
#ifdef SF2_VITA_TARGET
static uint32_t guard;
/* Only std-stream tokens at known ARM32 __sF offsets are supported. Direct
   FILE-structure access and arbitrary Android FILE objects are unsupported. */
static union {uint32_t alignment;unsigned char bytes[3*84];} streams;
static uint32_t module_base,exidx_base,exidx_count;
static size_t module_bytes;
static SfPreloadedDl module_loader;
void sf_libmain_runtime_namespace(ArmNamespace *ns){sf_preloaded_dl_init(&module_loader,ns);}
void sf_libmain_runtime_bind(uint32_t base,size_t bytes,uint32_t exidx,uint32_t count){
    module_base=base;module_bytes=bytes;exidx_base=exidx;exidx_count=count;
}
int sf_libmain_runtime_prepare(void){
    int rc=sceKernelGetRandomNumber(&guard,sizeof guard);if(rc<0)return rc;
    if(!guard)return -71;
    return 0;
}
static SF_ANDROID_ABI void terminal(void){
    FILE *f=fopen("ux0:data/sf2vita-input.log","a");
    if(f){fputs("LIBMAIN_RUNTIME terminal abort requested\n",f);fflush(f);fclose(f);}abort();
}
static SF_ANDROID_ABI int *android_errno(void){return &errno;}
static SF_ANDROID_ABI long android_strtol(const char *s,char **end,int base){return strtol(s,end,base);}
static SF_ANDROID_ABI int android_snprintf(char *dst,size_t count,const char *format,...){
    va_list ap;va_start(ap,format);int rc=vsnprintf(dst,count,format,ap);va_end(ap);return rc;
}
static SF_ANDROID_ABI void *android_memmem(const void *haystack,size_t length,const void *needle,size_t bytes){
    if(!bytes)return (void *)haystack;
    if(bytes>length)return 0;
    const unsigned char *p=haystack;
    for(size_t i=0;i<=length-bytes;i++)if(!memcmp(p+i,needle,bytes))return (void *)(p+i);
    return 0;
}
static SF_ANDROID_ABI int android_open(const char *path,int flags,...){
    /* libmain CPU-info reads use O_RDONLY. Other Android flag contracts are
       rejected, never passed blindly into newlib's differently numbered flags. */
    if(flags&~0x80000){errno=22;return -1;}return open(path,O_RDONLY);
}
static SF_ANDROID_ABI int android_close(int fd){return close(fd);}
static SF_ANDROID_ABI ssize_t android_read(int fd,void *data,size_t bytes){return read(fd,data,bytes);}
static FILE *stream(void *token){
    if(token==streams.bytes)return stdin;
    if(token==streams.bytes+84)return stdout;
    if(token==streams.bytes+168)return stderr;
    errno=9;return 0;
}
static SF_ANDROID_ABI int android_fprintf(void *token,const char *format,...){
    FILE *f=stream(token);if(!f)return -1;va_list ap;va_start(ap,format);
    int rc=vfprintf(f,format,ap);va_end(ap);return rc;
}
static SF_ANDROID_ABI int android_fflush(void *token){
    if(!token)return fflush(0);
    FILE *f=stream(token);return f?fflush(f):-1;
}
/* No filesystem loading: only an explicitly supplied, ready module registry. */
static SF_ANDROID_ABI void *android_dlopen(const char *path,int flags){
    return (void *)(uintptr_t)sf_preloaded_dlopen(&module_loader,path,flags);
}
static SF_ANDROID_ABI void *android_dlsym(void *handle,const char *name){
    return (void *)(uintptr_t)sf_preloaded_dlsym(&module_loader,(uint32_t)(uintptr_t)handle,name);
}
static SF_ANDROID_ABI int android_dlclose(void *handle){
    return sf_preloaded_dlclose(&module_loader,(uint32_t)(uintptr_t)handle);
}
static SF_ANDROID_ABI const char *android_dlerror(void){return sf_preloaded_dlerror(&module_loader);}

typedef struct {const char *name;void *base;const char *symbol;void *address;} AndroidDlInfo;
static SF_ANDROID_ABI int android_dladdr(const void *address,AndroidDlInfo *out){
    if(!out)return 0;
    memset(out,0,sizeof(*out));uintptr_t p=(uintptr_t)address;
    if(!module_base||p<module_base||p-module_base>=module_bytes)return 0;
    out->name="libmain.so";out->base=(void *)(uintptr_t)module_base;return 1;
}
static SF_ANDROID_ABI uintptr_t find_exidx(uintptr_t pc,int *count){
    if(count)*count=0;
    if(!count||!module_base||pc<module_base||pc-module_base>=module_bytes)return 0;
    *count=(int)exidx_count;return exidx_base;
}
int sf_libmain_runtime_catalog(ArmScopeEntry *out,size_t cap){
    if(!out||cap<22||sizeof(uintptr_t)!=4||sizeof(long)!=4)return -22;
#define F(n,f) (ArmScopeEntry){"host:libmain:entry",n,(uint32_t)(uintptr_t)&f,2,1}
#define D(n,p) (ArmScopeEntry){"host:libmain:entry",n,(uint32_t)(uintptr_t)p,1,1}
    out[0]=F("__cxa_atexit",sf_runtime_atexit);out[1]=F("__cxa_finalize",sf_runtime_finalize);
    out[2]=F("__stack_chk_fail",terminal);out[3]=D("__stack_chk_guard",&guard);
    out[4]=F("dlclose",android_dlclose);out[5]=F("dlerror",android_dlerror);
    out[6]=F("read",android_read);out[7]=F("dlopen",android_dlopen);
    out[8]=F("pthread_once",sf_bionic_pthread_once);out[9]=F("dlsym",android_dlsym);
    out[10]=F("snprintf",android_snprintf);out[11]=F("strtol",android_strtol);
    out[12]=F("__errno",android_errno);out[13]=F("close",android_close);
    out[14]=F("memmem",android_memmem);out[15]=F("open",android_open);
    out[16]=D("__sF",streams.bytes);out[17]=F("abort",terminal);
    out[18]=F("fprintf",android_fprintf);out[19]=F("fflush",android_fflush);
    out[20]=F("__gnu_Unwind_Find_exidx",find_exidx);out[21]=F("dladdr",android_dladdr);
#undef F
#undef D
    return 22;
}
#else
void sf_libmain_runtime_namespace(ArmNamespace *ns){(void)ns;}
int sf_libmain_runtime_prepare(void){return -22;}
void sf_libmain_runtime_bind(uint32_t a,size_t b,uint32_t c,uint32_t d){(void)a;(void)b;(void)c;(void)d;}
int sf_libmain_runtime_catalog(ArmScopeEntry *a,size_t b){(void)a;(void)b;return -22;}
#endif
