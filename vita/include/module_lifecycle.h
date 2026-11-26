#ifndef SF_MODULE_LIFECYCLE_H
#define SF_MODULE_LIFECYCLE_H
#include "arm_namespace.h"
#include "arm_lifecycle.h"
enum { SF_MODULE_BOUND=1,SF_MODULE_RUNNING_INIT,SF_MODULE_INITIALIZED,
       SF_MODULE_RUNNING_FINI,SF_MODULE_STOPPED,SF_MODULE_POISONED };
/* Explicit dispatcher: caller supplies the real ABI/runtime invocation.
   Returning zero means the callback returned successfully. No implicit cast
   of arbitrary integer addresses to host function pointers occurs here. */
typedef int (*SfModuleInvoke)(void *,uint32_t,unsigned,size_t);
typedef struct {
    ArmNamespace *ns;uint32_t handle;const ArmExecImage *image;
    ArmLifecyclePlan plan;
    SfModuleInvoke invoke;void *context;
    size_t init_attempted,init_completed,fini_attempted,fini_completed;
    unsigned state;
} SfModuleLifecycle;
/* Fresh owner-thread controller only. Copies and validates the complete plan.
   Namespace, image and dispatcher context must outlive the controller. */
int sf_module_lifecycle_bind(SfModuleLifecycle *,ArmNamespace *,uint32_t,
                             const ArmExecImage *,const ArmLifecyclePlan *,SfModuleInvoke,void *);
/* One-shot, no automatic rollback/retry. A callback failure poisons the module;
   mapping and dependencies remain retained for explicit recovery/process exit.
   Crash/longjmp/throw across dispatcher boundaries are not recovered. */
int sf_module_initialize(SfModuleLifecycle *);
int sf_module_finalize(SfModuleLifecycle *);
#endif
