#include "arm_namespace.h"
#include "arm_dynamic.h"
#include <stdio.h>
#include <string.h>
static int fail(ArmNamespace *ns,int rc,const char *message){
    if(ns){snprintf(ns->error,sizeof ns->error,"%s",message);ns->error_pending=1;}return rc;
}
static ArmNsModule *lookup(ArmNamespace *ns,uint32_t handle){
    if(!ns)return 0;
    unsigned i=handle&255u;uint32_t generation=handle>>8;
    if(!i||i>ARM_NS_MODULES||!generation)return 0;
    ArmNsModule *m=&ns->modules[i-1];
    return m->state&&m->generation==generation?m:0;
}
static int valid_name(const char *s){
    if(!s||!*s)return 0;
    for(size_t i=0;i<ARM_NS_NAME;i++){
        unsigned char c=(unsigned char)s[i];if(!c)return 1;
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-'))return 0;
    }return 0;
}
void arm_namespace_init(ArmNamespace *ns){if(ns)memset(ns,0,sizeof(*ns));}
int arm_namespace_add(ArmNamespace *ns,const char *name,const unsigned char *file,size_t size,
                      const ArmExecImage *image,const uint32_t *deps,size_t count,uint32_t *handle){
    if(handle)*handle=0;
    if(!ns||!handle||!valid_name(name)||!file||!image||!image->ready||
       count>ARM_NS_MODULES||(!deps&&count))return fail(ns,-90,"invalid module registration");
    ArmDynamicInfo info;
    if(arm_dynamic_read(file,size,0,0,&info)||image->size!=(size_t)info.load_end-info.load_min||
       image->bias!=image->base-info.load_min||(uint64_t)image->base+image->size>UINT32_MAX)return fail(ns,-97,"file/mapping mismatch");
    size_t slot=ARM_NS_MODULES;
    for(size_t i=0;i<ARM_NS_MODULES;i++){
        ArmNsModule *m=&ns->modules[i];
        if(m->state&&(!strcmp(m->name,name)||m->image==image))return fail(ns,-92,"module already registered");
        if(m->state&&(uint64_t)image->base+image->size>m->image->base&&
           (uint64_t)m->image->base+m->image->size>image->base)return fail(ns,-92,"overlapping module mappings");
        if(!m->state&&m->generation<0xffffffu&&slot==ARM_NS_MODULES)slot=i;
    }
    if(slot==ARM_NS_MODULES)return fail(ns,-94,"module registry full");
    if(ns->serial==UINT64_MAX)return fail(ns,-96,"registration order exhausted");
    for(size_t i=0;i<count;i++){
        ArmNsModule *dep=lookup(ns,deps[i]);
        if(!dep||dep->state==ARM_NS_FAILED||dep->state==ARM_NS_FINALIZING||dep->state==ARM_NS_FINISHED)
            return fail(ns,-95,"dependency unavailable");
        if(dep->pins==UINT32_MAX)return fail(ns,-96,"dependency reference overflow");
        for(size_t j=0;j<i;j++)if(deps[i]==deps[j])return fail(ns,-90,"duplicate dependency");
    }
    ArmNsModule *m=&ns->modules[slot];uint32_t generation=m->generation+1;
    memset(m,0,sizeof(*m));m->generation=generation;m->state=ARM_NS_LINKED;
    strcpy(m->name,name);m->file=file;m->file_size=size;m->image=image;m->order=++ns->serial;
    m->dependency_count=(unsigned)count;
    for(size_t i=0;i<count;i++){m->dependencies[i]=deps[i];lookup(ns,deps[i])->pins++;}
    *handle=(generation<<8)|(uint32_t)(slot+1);return 0;
}
int arm_namespace_remove(ArmNamespace *ns,uint32_t handle){
    ArmNsModule *m=lookup(ns,handle);if(!m)return fail(ns,-91,"invalid module handle");
    if(m->references||m->pins)return fail(ns,-92,"module still referenced");
    if(m->state==ARM_NS_INITIALIZING||m->state==ARM_NS_FINALIZING||m->state==ARM_NS_READY)
        return fail(ns,-93,"module lifecycle not stopped");
    for(unsigned i=0;i<m->dependency_count;i++){
        ArmNsModule *dep=lookup(ns,m->dependencies[i]);if(dep&&dep->pins)dep->pins--;
    }
    uint32_t generation=m->generation;memset(m,0,sizeof(*m));m->generation=generation;return 0;
}
int arm_namespace_begin_init(ArmNamespace *ns,uint32_t handle){
    ArmNsModule *m=lookup(ns,handle);if(!m)return fail(ns,-91,"invalid module handle");
    if(m->state!=ARM_NS_LINKED)return fail(ns,-93,"module cannot initialize");
    for(unsigned i=0;i<m->dependency_count;i++){
        ArmNsModule *dep=lookup(ns,m->dependencies[i]);
        if(!dep||dep->state!=ARM_NS_READY)return fail(ns,-95,"dependency not initialized");
    }
    m->state=ARM_NS_INITIALIZING;return 0;
}
int arm_namespace_end_init(ArmNamespace *ns,uint32_t handle,int success){
    ArmNsModule *m=lookup(ns,handle);if(!m)return fail(ns,-91,"invalid module handle");
    if(m->state!=ARM_NS_INITIALIZING)return fail(ns,-93,"initialization not active");
    m->state=success?ARM_NS_READY:ARM_NS_FAILED;return 0;
}
int arm_namespace_begin_fini(ArmNamespace *ns,uint32_t handle){
    ArmNsModule *m=lookup(ns,handle);if(!m)return fail(ns,-91,"invalid module handle");
    if(m->references||m->pins)return fail(ns,-92,"module still referenced");
    if(m->state!=ARM_NS_READY)return fail(ns,-93,"module cannot finalize");
    m->state=ARM_NS_FINALIZING;return 0;
}
int arm_namespace_end_fini(ArmNamespace *ns,uint32_t handle,int success){
    ArmNsModule *m=lookup(ns,handle);if(!m)return fail(ns,-91,"invalid module handle");
    if(m->state!=ARM_NS_FINALIZING)return fail(ns,-93,"finalization not active");
    m->state=success?ARM_NS_FINISHED:ARM_NS_FAILED;return 0;
}
int arm_namespace_open(ArmNamespace *ns,const char *name,uint32_t *handle){
    if(handle)*handle=0;
    if(!ns||!handle||!valid_name(name))return fail(ns,-90,"invalid module name");
    for(unsigned i=0;i<ARM_NS_MODULES;i++){
        ArmNsModule *m=&ns->modules[i];if(!m->state||strcmp(m->name,name))continue;
        if(m->state!=ARM_NS_READY)return fail(ns,-93,"module not ready");
        if(m->references==UINT32_MAX)return fail(ns,-96,"module reference overflow");
        m->references++;*handle=(m->generation<<8)|(i+1);return 0;
    }return fail(ns,-91,"module not registered");
}
int arm_namespace_close(ArmNamespace *ns,uint32_t handle){
    ArmNsModule *m=lookup(ns,handle);if(!m)return fail(ns,-91,"invalid module handle");
    if(!m->references)return fail(ns,-93,"module handle not open");
    m->references--;return 0;
}
int arm_namespace_symbol(ArmNamespace *ns,uint32_t handle,const char *name,ArmExport *out){
    if(out)memset(out,0,sizeof(*out));
    ArmNsModule *m=lookup(ns,handle);if(!m)return fail(ns,-91,"invalid module handle");
    if(!out||!name||!*name)return fail(ns,-90,"invalid symbol lookup");
    if(m->state!=ARM_NS_READY||!m->references)return fail(ns,-93,"module handle not open and ready");
    /* Breadth-first, explicitly declared dependency closure. Pins preserve
       mappings; each registered handle enters the queue only once. */
    uint32_t queue[ARM_NS_MODULES];size_t head=0,tail=1;queue[0]=handle;
    while(head<tail){
        ArmNsModule *current=lookup(ns,queue[head++]);
        if(!current||current->state!=ARM_NS_READY)return fail(ns,-95,"dependency not ready for symbol lookup");
        int rc=arm_symbol_find(current->file,current->file_size,name,current->image->bias,out);
        if(rc<0)return fail(ns,rc,"malformed module exports");
        if(rc)return 1;
        for(unsigned i=0;i<current->dependency_count;i++){
            uint32_t dep=current->dependencies[i];int seen=0;
            for(size_t j=0;j<tail;j++)if(queue[j]==dep){seen=1;break;}
            if(!seen){
                if(tail==ARM_NS_MODULES)return fail(ns,-95,"dependency closure overflow");
                queue[tail++]=dep;
            }
        }
    }
    return fail(ns,-91,"symbol not found");
}
int arm_namespace_resolve(const char *name,unsigned binding,unsigned type,uint32_t *value,void *ctx){
    ArmNamespace *ns=ctx;
    if(!ns||!name||!*name||!value||type>2||(binding!=1&&binding!=2))return fail(ns,-90,"invalid relocation lookup");
    uint64_t cursor=0;
    for(unsigned pass=0;pass<ARM_NS_MODULES;pass++){
        ArmNsModule *chosen=0;
        for(unsigned i=0;i<ARM_NS_MODULES;i++){
            ArmNsModule *m=&ns->modules[i];
            if(m->state<ARM_NS_LINKED||m->state>ARM_NS_READY||m->order<=cursor)continue;
            if(!chosen||m->order<chosen->order)chosen=m;
        }
        if(!chosen)return 0;
        cursor=chosen->order;ArmExport found;
        int rc=arm_symbol_find(chosen->file,chosen->file_size,name,chosen->image->bias,&found);
        if(rc<0)return fail(ns,rc,"malformed module exports");
        if(rc){
            if(type&&found.type&&type!=found.type)return fail(ns,-98,"symbol type mismatch");
            *value=found.value;return 1;
        }
    }return 0;
}
const char *arm_namespace_error(ArmNamespace *ns){
    if(!ns||!ns->error_pending)return 0;
    ns->error_pending=0;return ns->error;
}
int arm_namespace_state(const ArmNamespace *ns,uint32_t handle){
    if(!ns)return 0;
    unsigned i=handle&255u;
    if(!i||i>ARM_NS_MODULES||!ns->modules[i-1].state||ns->modules[i-1].generation!=(handle>>8))return 0;
    return (int)ns->modules[i-1].state;
}
