#include "module_lifecycle.h"
#include <string.h>
static int target_valid(const ArmExecImage *image,uint32_t address){
    uint32_t raw=address&~1u;unsigned width=address&1?2:4;
    if(!address||(!(address&1)&&(raw&3))||raw<image->base||
       (uint64_t)raw+width>(uint64_t)image->base+image->size)return 0;
    for(unsigned i=0;i<image->code_count;i++)if(raw>=image->code[i].begin&&
       (uint64_t)raw+width<=image->code[i].end)return 1;
    return 0;
}
static int view_valid(const SfModuleLifecycle *run){
    if(!run||!run->ns||!run->image||!run->image->ready||!run->invoke)return 0;
    unsigned slot=run->handle&255u;
    if(!slot||slot>ARM_NS_MODULES||!arm_namespace_state(run->ns,run->handle))return 0;
    return run->ns->modules[slot-1].image==run->image&&
           run->image->base==run->plan.base&&run->image->size==run->plan.image_size;
}
int sf_module_lifecycle_bind(SfModuleLifecycle *run,ArmNamespace *ns,uint32_t handle,
                             const ArmExecImage *image,const ArmLifecyclePlan *plan,SfModuleInvoke invoke,void *ctx){
    if(!run)return -110;
    memset(run,0,sizeof(*run));
    if(!ns||!image||!plan||!invoke||!plan->validated||!image->ready||
       !image->code_count||image->code_count>ARM_EXEC_MAX_RANGES||
       plan->init_count>ARM_LIFECYCLE_MAX||plan->fini_count>ARM_LIFECYCLE_MAX||
       plan->base!=image->base||plan->image_size!=image->size||
       (uint64_t)image->base+image->size>UINT32_MAX||
       arm_namespace_state(ns,handle)!=ARM_NS_LINKED)return -110;
    unsigned slot=handle&255u;
    if(ns->modules[slot-1].image!=image)return -110;
    for(size_t i=0;i<plan->init_count;i++)if(!target_valid(image,plan->init[i]))return -111;
    for(size_t i=0;i<plan->fini_count;i++)if(!target_valid(image,plan->fini[i]))return -111;
    run->ns=ns;run->handle=handle;run->image=image;run->plan=*plan;
    run->invoke=invoke;run->context=ctx;run->state=SF_MODULE_BOUND;return 0;
}
int sf_module_initialize(SfModuleLifecycle *run){
    if(!view_valid(run)||run->state!=SF_MODULE_BOUND)return -112;
    int rc=arm_namespace_begin_init(run->ns,run->handle);if(rc)return rc;
    run->state=SF_MODULE_RUNNING_INIT;
    for(size_t i=0;i<run->plan.init_count;i++){
        run->init_attempted++;
        rc=run->invoke(run->context,run->plan.init[i],0,i);
        if(rc){arm_namespace_end_init(run->ns,run->handle,0);run->state=SF_MODULE_POISONED;return rc;}
        run->init_completed++;
    }
    rc=arm_namespace_end_init(run->ns,run->handle,1);
    run->state=rc?SF_MODULE_POISONED:SF_MODULE_INITIALIZED;return rc;
}
int sf_module_finalize(SfModuleLifecycle *run){
    if(!view_valid(run)||run->state!=SF_MODULE_INITIALIZED)return -112;
    int rc=arm_namespace_begin_fini(run->ns,run->handle);if(rc)return rc;
    run->state=SF_MODULE_RUNNING_FINI;
    for(size_t i=0;i<run->plan.fini_count;i++){
        run->fini_attempted++;
        rc=run->invoke(run->context,run->plan.fini[i],1,i);
        if(rc){arm_namespace_end_fini(run->ns,run->handle,0);run->state=SF_MODULE_POISONED;return rc;}
        run->fini_completed++;
    }
    rc=arm_namespace_end_fini(run->ns,run->handle,1);
    run->state=rc?SF_MODULE_POISONED:SF_MODULE_STOPPED;return rc;
}
