#include "aeabi_memory.h"
#include <stdint.h>
/* Host semantic reference only. ARM implementations are in aeabi_memory.S:
   ordinary libc wrappers may corrupt VFP registers forbidden by RTABI. */
#ifndef __arm__
#include <string.h>
#define COPY(n,fn) SF_ANDROID_ABI void sf_aeabi_##n(void *d,const void *s,size_t nbytes){if(nbytes)fn(d,s,nbytes);}
COPY(memcpy,memcpy) COPY(memcpy4,memcpy) COPY(memcpy8,memcpy)
COPY(memmove,memmove) COPY(memmove4,memmove) COPY(memmove8,memmove)
#define SET(n) SF_ANDROID_ABI void sf_aeabi_##n(void *d,size_t bytes,int c){if(bytes)memset(d,c,bytes);}
SET(memset) SET(memset4) SET(memset8)
#define CLR(n) SF_ANDROID_ABI void sf_aeabi_##n(void *d,size_t bytes){if(bytes)memset(d,0,bytes);}
CLR(memclr) CLR(memclr4) CLR(memclr8)
#endif
int sf_aeabi_memory_catalog(ArmScopeEntry *out,size_t capacity){
#if defined(SF2_VITA_TARGET) && defined(__arm__)
    if(!out||capacity<12||sizeof(uintptr_t)!=4||sizeof(size_t)!=4)return -22;
#define E(n) (ArmScopeEntry){"host:arm:rtabi","__aeabi_"#n,(uint32_t)(uintptr_t)&sf_aeabi_##n,2,1}
    out[0]=E(memcpy);out[1]=E(memcpy4);out[2]=E(memcpy8);
    out[3]=E(memmove);out[4]=E(memmove4);out[5]=E(memmove8);
    out[6]=E(memset);out[7]=E(memset4);out[8]=E(memset8);
    out[9]=E(memclr);out[10]=E(memclr4);out[11]=E(memclr8);
#undef E
    return 12;
#else
    (void)out;(void)capacity;return -22;
#endif
}
