#include "native_registry.h"
#include <string.h>
static int same(const NativeBinding *b,const char *c,const char *n,const char *d){
    return !strcmp(b->class_name,c)&&!strcmp(b->name,n)&&!strcmp(b->descriptor,d);
}
int native_bind(NativeRegistry *r,const char *c,const char *n,const char *d,void (*fn)(void)){
    if(!r||r->count>128||!c||!*c||!n||!*n||!d||!*d||!fn)return -1;
    for(size_t i=0;i<r->count;i++)if(same(&r->entries[i],c,n,d))return r->entries[i].address==fn?0:-2;
    if(r->count==128)return -3;
    r->entries[r->count++]=(NativeBinding){c,n,d,fn};return 0;
}
void (*native_lookup(const NativeRegistry *r,const char *c,const char *n,const char *d))(void){
    if(!r||r->count>128||!c||!n||!d)return 0;
    for(size_t i=0;i<r->count;i++)if(same(&r->entries[i],c,n,d))return r->entries[i].address;
    return 0;
}
int native_register_checked(NativeRegistry *r,const NativeDeclaration *decl,size_t nd,
                            const NativeBinding *batch,size_t nb){
    if(!r||r->count>128||!decl||!nd||!batch||!nb||nb>128)return -1;
    for(size_t j=0;j<nd;j++)
        if(!decl[j].class_name||!decl[j].name||!decl[j].descriptor)return -1;
    NativeRegistry staged=*r;
    for(size_t i=0;i<nb;i++){
        const NativeBinding *b=&batch[i];int found=0;
        if(!b->class_name||!b->name||!b->descriptor)return -1;
        for(size_t j=0;j<nd;j++)
            if(!strcmp(b->class_name,decl[j].class_name)&&!strcmp(b->name,decl[j].name)&&
               !strcmp(b->descriptor,decl[j].descriptor)){found=1;break;}
        if(!found)return -4;
        int rc=native_bind(&staged,b->class_name,b->name,b->descriptor,b->address);
        if(rc)return rc;
    }
    *r=staged;return 0;
}
