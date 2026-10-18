#include "bionic_memory.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
SF_ANDROID_ABI void *sf_bionic_malloc(size_t n){return malloc(n);}
SF_ANDROID_ABI void *sf_bionic_calloc(size_t n,size_t size){
    if(size&&n>SIZE_MAX/size)return 0;
    return calloc(n,size);
}
SF_ANDROID_ABI void *sf_bionic_realloc(void *p,size_t n){return realloc(p,n);}
SF_ANDROID_ABI void sf_bionic_free(void *p){free(p);}
SF_ANDROID_ABI void *sf_bionic_memcpy(void *d,const void *s,size_t n){return memcpy(d,s,n);}
SF_ANDROID_ABI void *sf_bionic_memmove(void *d,const void *s,size_t n){return memmove(d,s,n);}
SF_ANDROID_ABI void *sf_bionic_memset(void *d,int v,size_t n){return memset(d,v,n);}
SF_ANDROID_ABI int sf_bionic_memcmp(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
SF_ANDROID_ABI void *sf_bionic_memchr(const void *p,int c,size_t n){return memchr(p,c,n);}
SF_ANDROID_ABI size_t sf_bionic_strlen(const char *s){return strlen(s);}
SF_ANDROID_ABI int sf_bionic_strcmp(const char *a,const char *b){return strcmp(a,b);}
SF_ANDROID_ABI int sf_bionic_strncmp(const char *a,const char *b,size_t n){return strncmp(a,b,n);}
SF_ANDROID_ABI char *sf_bionic_strchr(const char *s,int c){return strchr(s,c);}
SF_ANDROID_ABI char *sf_bionic_strrchr(const char *s,int c){return strrchr(s,c);}
SF_ANDROID_ABI char *sf_bionic_strstr(const char *s,const char *p){return strstr(s,p);}
SF_ANDROID_ABI char *sf_bionic_strcpy(char *d,const char *s){return strcpy(d,s);}
SF_ANDROID_ABI char *sf_bionic_strncpy(char *d,const char *s,size_t n){return strncpy(d,s,n);}
SF_ANDROID_ABI char *sf_bionic_strcat(char *d,const char *s){return strcat(d,s);}
SF_ANDROID_ABI char *sf_bionic_strncat(char *d,const char *s,size_t n){return strncat(d,s,n);}
SF_ANDROID_ABI char *sf_bionic_strdup(const char *s){
    size_t n=strlen(s);if(n==SIZE_MAX)return 0;
    char *p=malloc(n+1);if(p)memcpy(p,s,n+1);return p;
}
int sf_bionic_memory_catalog(ArmScopeEntry *out,size_t cap){
#if defined(SF2_VITA_TARGET) && defined(__arm__)
    if(!out||cap<20||sizeof(uintptr_t)!=4||sizeof(size_t)!=4)return -22;
#define E(n) (ArmScopeEntry){"host:bionic:memory",#n,(uint32_t)(uintptr_t)&sf_bionic_##n,2,1}
    out[0]=E(malloc);out[1]=E(calloc);out[2]=E(realloc);out[3]=E(free);
    out[4]=E(memcpy);out[5]=E(memmove);out[6]=E(memset);out[7]=E(memcmp);out[8]=E(memchr);
    out[9]=E(strlen);out[10]=E(strcmp);out[11]=E(strncmp);out[12]=E(strchr);out[13]=E(strrchr);
    out[14]=E(strstr);out[15]=E(strcpy);out[16]=E(strncpy);out[17]=E(strcat);out[18]=E(strncat);out[19]=E(strdup);
#undef E
    return 20;
#else
    (void)out;(void)cap;return -22;
#endif
}
