#include "bionic_mutex.h"
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
#include <sched.h>
#define MUTEX_COUNT 512
#define GENERATION_MAX 0x1fffffu
typedef struct {pthread_mutex_t native;int32_t *owner;uint32_t generation,refs,held;int live;} Mutex;
static Mutex mutexes[MUTEX_COUNT];
static unsigned registry_lock;
static void enter(void){while(__atomic_exchange_n(&registry_lock,1,__ATOMIC_ACQUIRE))sched_yield();}
static void leave(void){__atomic_store_n(&registry_lock,0,__ATOMIC_RELEASE);}
static int aligned(const void *p){return p&&!(3u&(uintptr_t)p);}
static int error(int rc){
    if(!rc)return 0;
    if(rc==EINVAL)return 22;
    if(rc==ENOMEM)return 12;
    if(rc==EBUSY)return 16;
    if(rc==EPERM)return 1;
    if(rc==EDEADLK)return 35;
    if(rc==EAGAIN)return 11;
    return 95;
}
SF_ANDROID_ABI int sf_bionic_mutexattr_init(int32_t *attr){if(!aligned(attr))return 22;*attr=0;return 0;}
SF_ANDROID_ABI int sf_bionic_mutexattr_destroy(int32_t *attr){if(!aligned(attr)||*attr<0||*attr>2)return 22;*attr=-1;return 0;}
SF_ANDROID_ABI int sf_bionic_mutexattr_settype(int32_t *attr,int type){
    if(!aligned(attr)||*attr<0||*attr>2||type<0||type>2)return 22;
    *attr=type;return 0;
}
static uint32_t token(unsigned i){return 0x80000000u|(mutexes[i].generation<<10)|(i+1);}
static Mutex *lookup(int32_t *control){
    uint32_t value=__atomic_load_n(control,__ATOMIC_RELAXED),index=value&1023u;
    if(!(value&0x80000000)||!index||index>MUTEX_COUNT)return 0;
    Mutex *m=&mutexes[index-1];return m->live&&m->owner==control&&token(index-1)==value?m:0;
}
/* Registry lock held. Publication of a new native object is atomic. */
static int create(int32_t *control,int type,Mutex **out){
    pthread_mutexattr_t attr;int rc=pthread_mutexattr_init(&attr);if(rc)return error(rc);
    int native_type=type==1?PTHREAD_MUTEX_RECURSIVE:type==2?PTHREAD_MUTEX_ERRORCHECK:PTHREAD_MUTEX_NORMAL;
    rc=pthread_mutexattr_settype(&attr,native_type);
    if(rc){pthread_mutexattr_destroy(&attr);return error(rc);}
    for(unsigned i=0;i<MUTEX_COUNT;i++){
        Mutex *m=&mutexes[i];if(m->live||m->generation==GENERATION_MAX)continue;
        rc=pthread_mutex_init(&m->native,&attr);pthread_mutexattr_destroy(&attr);if(rc)return error(rc);
        m->generation++;m->refs=0;m->held=0;m->owner=control;m->live=1;
        __atomic_store_n(control,(int32_t)token(i),__ATOMIC_RELEASE);*out=m;return 0;
    }
    pthread_mutexattr_destroy(&attr);return 12;
}
SF_ANDROID_ABI int sf_bionic_mutex_init(int32_t *control,const int32_t *attr){
    if(!aligned(control)||(attr&&(!aligned(attr)||*attr<0||*attr>2)))return 22;
    enter();Mutex *m=0;
    for(unsigned i=0;i<MUTEX_COUNT;i++)if(mutexes[i].live&&mutexes[i].owner==control){m=&mutexes[i];break;}
    int rc=m?16:create(control,attr?*attr:0,&m);leave();return rc;
}
static int lease(int32_t *control,Mutex **out){
    if(!aligned(control))return 22;
    enter();Mutex *m=lookup(control);int rc=0;
    if(!m){
        uint32_t value=__atomic_load_n(control,__ATOMIC_RELAXED);
        if(value!=0&&value!=0x4000&&value!=0x8000){leave();return 22;}
        rc=create(control,value==0x4000?1:value==0x8000?2:0,&m);
    }
    if(!rc){m->refs++;*out=m;}leave();return rc;
}
static void unlease(Mutex *m,int delta){enter();if(delta>0)m->held++;else if(delta<0&&m->held)m->held--;m->refs--;leave();}
SF_ANDROID_ABI int sf_bionic_mutex_lock(int32_t *control){
    Mutex *m;int rc=lease(control,&m);if(rc)return rc;
    rc=pthread_mutex_lock(&m->native);unlease(m,rc?0:1);return error(rc);
}
SF_ANDROID_ABI int sf_bionic_mutex_trylock(int32_t *control){
    Mutex *m;int rc=lease(control,&m);if(rc)return rc;
    rc=pthread_mutex_trylock(&m->native);unlease(m,rc?0:1);return error(rc);
}
SF_ANDROID_ABI int sf_bionic_mutex_unlock(int32_t *control){
    if(!aligned(control))return 22;
    enter();Mutex *m=lookup(control);if(!m){leave();return 22;}m->refs++;leave();
    int rc=pthread_mutex_unlock(&m->native);unlease(m,rc?0:-1);return error(rc);
}
SF_ANDROID_ABI int sf_bionic_mutex_destroy(int32_t *control){
    if(!aligned(control))return 22;
    enter();Mutex *m=lookup(control);
    if(!m){uint32_t value=__atomic_load_n(control,__ATOMIC_RELAXED);
        if(value==0||value==0x4000||value==0x8000){__atomic_store_n(control,-1,__ATOMIC_RELEASE);leave();return 0;}
        leave();return 22;
    }
    if(m->refs||m->held){leave();return 16;}
    int rc=pthread_mutex_destroy(&m->native);
    if(!rc){m->live=0;m->owner=0;__atomic_store_n(control,-1,__ATOMIC_RELEASE);}
    leave();return error(rc);
}
