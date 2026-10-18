#include "bionic_once.h"
#include <stdint.h>
#ifdef SF2_VITA_TARGET
#include <psp2/kernel/threadmgr.h>
static void wait_tick(void){sceKernelDelayThread(1000);}
#elif defined(_WIN32)
#include <windows.h>
static void wait_tick(void){Sleep(1);}
#else
#include <sched.h>
static void wait_tick(void){sched_yield();}
#endif
SF_ANDROID_ABI int sf_bionic_pthread_once(volatile int32_t *control,SfOnceInit init){
    if(!control||!init||((uintptr_t)control&3))return 22;
    for(;;){
        int32_t state=__atomic_load_n(control,__ATOMIC_ACQUIRE);
        if(state==2)return 0;
        if(state!=0&&state!=1)return 22;
        if(!state){
            int32_t expected=0;
            if(__atomic_compare_exchange_n(control,&expected,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED)){
                init();__atomic_store_n(control,2,__ATOMIC_RELEASE);return 0;
            }
        }
        wait_tick();
    }
}
