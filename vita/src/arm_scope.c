#include "arm_scope.h"
#include <string.h>
int arm_scope_init(ArmSymbolScope *scope,const ArmScopeEntry *entries,size_t count){
    if(!scope)return -1;
    scope->entries=0;scope->count=0;
    if((!entries&&count)||count>SIZE_MAX/sizeof(*entries))return -1;
    for(size_t i=0;i<count;i++){
        const ArmScopeEntry *e=&entries[i];
        if(!e->provider||!*e->provider||!e->name||!*e->name||e->type>2||
           (e->binding!=1&&e->binding!=2)||(e->type==2&&!e->value))return -2;
        for(size_t j=0;j<i;j++)if(!strcmp(entries[j].provider,e->provider)&&!strcmp(entries[j].name,e->name))return -3;
    }
    scope->entries=entries;scope->count=count;return 0;
}
int arm_scope_find(const ArmSymbolScope *scope,const char *name,unsigned type,const ArmScopeEntry **selected){
    if(selected)*selected=0;
    if(!scope||!name||!*name||!selected||type>2||(!scope->entries&&scope->count))return -1;
    for(size_t i=0;i<scope->count;i++){
        const ArmScopeEntry *e=&scope->entries[i];
        if(strcmp(e->name,name))continue;
        if(type&&e->type&&type!=e->type)return -4;
        *selected=e;return 1;
    }
    return 0;
}
int arm_scope_resolve(const char *name,unsigned binding,unsigned type,uint32_t *value,void *context){
    if(!value||(binding!=1&&binding!=2))return -1;
    const ArmScopeEntry *selected=0;
    int rc=arm_scope_find((const ArmSymbolScope *)context,name,type,&selected);
    if(rc==1)*value=selected->value;
    return rc;
}
