#include "jni_registration.h"
#include <stdlib.h>
#include <string.h>
static int fits(const char *s,size_t cap){
    if(!s||!*s)return 0;
    for(size_t i=0;i<cap;i++)if(!s[i])return 1;
    return 0;
}
static int matches(const SfRegisteredNative *m,const char *c,const char *n,const char *s){
    return !strcmp(m->class_name,c)&&!strcmp(m->name,n)&&!strcmp(m->signature,s);
}
uintptr_t sf_jni_registered(const SfNativeCatalog *c,const char *cls,const char *name,const char *sig){
    if(!c||c->count>128||!cls||!name||!sig)return 0;
    for(size_t i=0;i<c->count;i++)if(matches(&c->entries[i],cls,name,sig))return c->entries[i].address;
    return 0;
}
static int code_allowed(uintptr_t ptr,const SfJniCodeRange *ranges,size_t count){
    if(!ptr||ptr>UINT32_MAX)return 0;
    uintptr_t address=ptr&~(uintptr_t)1;size_t width=ptr&1?2:4;
    if(!(ptr&1)&&address%4)return 0;
    for(size_t i=0;i<count;i++)
        if(ranges[i].begin<=address&&address<ranges[i].end&&width<=ranges[i].end-address)return 1;
    return 0;
}
int sf_jni_register_batch(SfNativeCatalog *catalog,const NativeDeclaration *decl,size_t nd,
                          const SfJniCodeRange *ranges,size_t nr,const char *cls,
                          const SfJniNativeMethod *methods,int32_t count){
    if(!catalog||catalog->count>128||!decl||!nd||!ranges||!nr||nr>16||
       !fits(cls,128)||!methods||count<=0||count>128)return -1;
    for(size_t i=0;i<nr;i++){
        if(ranges[i].begin>=ranges[i].end)return -1;
#if UINTPTR_MAX > UINT32_MAX
        if(ranges[i].end>UINT64_C(0x100000000))return -1;
#endif
    }
    for(size_t i=0;i<nd;i++)if(!decl[i].class_name||!decl[i].name||!decl[i].descriptor)return -1;
    SfNativeCatalog *staged=malloc(sizeof(*staged));if(!staged)return -3;
    *staged=*catalog;int rc=0;
    for(int32_t i=0;i<count;i++){
        const SfJniNativeMethod *m=&methods[i];int declared=0;
        if(!fits(m->name,128)||!fits(m->signature,512)){rc=-1;break;}
        for(size_t j=0;j<nd;j++)if(!strcmp(cls,decl[j].class_name)&&
            !strcmp(m->name,decl[j].name)&&!strcmp(m->signature,decl[j].descriptor)){declared=1;break;}
        if(!declared){rc=-4;break;}
        uintptr_t address=(uintptr_t)m->fnPtr;
        if(!code_allowed(address,ranges,nr)){rc=-5;break;}
        uintptr_t previous=sf_jni_registered(staged,cls,m->name,m->signature);
        if(previous){if(previous!=address){rc=-2;break;}continue;}
        if(staged->count==128){rc=-3;break;}
        SfRegisteredNative *out=&staged->entries[staged->count++];
        strcpy(out->class_name,cls);strcpy(out->name,m->name);strcpy(out->signature,m->signature);out->address=address;
    }
    if(!rc)*catalog=*staged;
    free(staged);return rc;
}
