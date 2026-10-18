#include "native_registry.h"
#include <string.h>
static int same(const NativeBinding *b,const char *c,const char *n,const char *d){
    return !strcmp(b->class_name,c)&&!strcmp(b->name,n)&&!strcmp(b->descriptor,d);
}
int native_bind(NativeRegistry *r,const char *c,const char *n,const char *d,void (*fn)(void)){
    if(!r||!c||!n||!d||!fn)return -1;
    for(size_t i=0;i<r->count;i++)if(same(&r->entries[i],c,n,d))return r->entries[i].address==fn?0:-2;
    if(r->count==128)return -3;
    r->entries[r->count++]=(NativeBinding){c,n,d,fn};return 0;
}
void (*native_lookup(const NativeRegistry *r,const char *c,const char *n,const char *d))(void){
    if(!r||!c||!n||!d)return 0;
    for(size_t i=0;i<r->count;i++)if(same(&r->entries[i],c,n,d))return r->entries[i].address;
    return 0;
}
