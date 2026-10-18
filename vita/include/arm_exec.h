#ifndef SF_ARM_EXEC_H
#define SF_ARM_EXEC_H
#include "arm_link.h"
#define ARM_EXEC_MAX_RANGES 8
/* Backend owns platform allocation. No callback may execute module code.
   address is the ARM32 virtual base; cookie is an opaque ownership token. */
typedef struct {
    void *context;
    int (*reserve)(void *,size_t,size_t,uint32_t *,uintptr_t *);
    int (*publish)(void *,uintptr_t,const unsigned char *,size_t);
    int (*sync)(void *,uintptr_t,size_t);
    void (*release)(void *,uintptr_t);
} ArmExecBackend;
typedef struct {
    ArmExecBackend backend;uintptr_t cookie;uint32_t base,bias;size_t size;
    struct {uint32_t begin,end;} code[ARM_EXEC_MAX_RANGES];
    unsigned code_count;int ready;
} ArmExecImage;
/* Initial executable path: no TLS, constructors or W+X segments accepted.
   Symbol resolution must fully succeed before publication/cache sync.
   This does NOT implement Android runtime semantics or call constructors. */
int arm_exec_map(const unsigned char *,size_t,size_t,ArmResolve,void *,
                 const ArmExecBackend *,ArmExecImage *,ArmLinkError *);
int arm_exec_entry(const ArmExecImage *,uint32_t,uint32_t *);
void arm_exec_release(ArmExecImage *);
#endif
