#ifndef SF_ARM_LIFECYCLE_H
#define SF_ARM_LIFECYCLE_H
#include "arm_link.h"
#define ARM_LIFECYCLE_MAX 1024
typedef struct {
    uint32_t init[ARM_LIFECYCLE_MAX],fini[ARM_LIFECYCLE_MAX];
    size_t init_count,fini_count;
    uint32_t base;size_t image_size;int validated;
} ArmLifecyclePlan;
/* DATA ONLY. Source file and staged image must be the immutable pair returned
   by arm_link_image. Only local R_ARM_RELATIVE array entries are supported.
   Validates ALL calls, including teardown, before publishing a plan. On error
   clears output. Does not call constructors or certify signatures/import ABI. */
int arm_lifecycle_plan(const unsigned char *,size_t,const ArmLinkedImage *,ArmLifecyclePlan *);
#endif
