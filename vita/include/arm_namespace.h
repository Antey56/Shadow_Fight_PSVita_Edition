#ifndef SF_ARM_NAMESPACE_H
#define SF_ARM_NAMESPACE_H
#include "arm_exec.h"
#include "arm_symbols.h"
#define ARM_NS_MODULES 16
#define ARM_NS_NAME 64
enum { ARM_NS_LINKED=1,ARM_NS_INITIALIZING,ARM_NS_READY,
       ARM_NS_FINALIZING,ARM_NS_FAILED,ARM_NS_FINISHED };
typedef struct {
    char name[ARM_NS_NAME];
    const unsigned char *file;size_t file_size;
    const ArmExecImage *image;
    uint32_t generation,references,pins,dependencies[ARM_NS_MODULES];
    unsigned dependency_count,state;
    uint64_t order;
} ArmNsModule;
typedef struct {
    ArmNsModule modules[ARM_NS_MODULES];uint64_t serial;
    char error[128];unsigned error_pending;
} ArmNamespace;
/* Owner-thread registry only. File bytes/image/storage are borrowed immutable
   until remove; owner releases mappings AFTER remove. No code is executed.
   Fresh namespace only: never reset with registered modules/handles. */
void arm_namespace_init(ArmNamespace *);
int arm_namespace_add(ArmNamespace *,const char *,const unsigned char *,size_t,
                      const ArmExecImage *,const uint32_t *,size_t,uint32_t *);
int arm_namespace_remove(ArmNamespace *,uint32_t);
/* Lifecycle transitions record caller-verified work; they never call constructors. */
int arm_namespace_begin_init(ArmNamespace *,uint32_t);
int arm_namespace_end_init(ArmNamespace *,uint32_t,int);
int arm_namespace_begin_fini(ArmNamespace *,uint32_t);
int arm_namespace_end_fini(ArmNamespace *,uint32_t,int);
/* Exact registered basename lookup, READY modules only. No filesystem loading,
   implicit constructors, RTLD_DEFAULT/NEXT, flags or Android namespace emulation. */
int arm_namespace_open(ArmNamespace *,const char *,uint32_t *);
int arm_namespace_close(ArmNamespace *,uint32_t);
/* Search the opened module, then its declared dependency closure breadth-first. */
int arm_namespace_symbol(ArmNamespace *,uint32_t,const char *,ArmExport *);
/* Explicit global registration-order scope for ELF relocation resolution.
   LINKED/INITIALIZING/READY exports are visible. NOT an Android lookup scope.
   A concrete type mismatch or malformed module is an error, not a miss. */
int arm_namespace_resolve(const char *,unsigned,unsigned,uint32_t *,void *);
/* Error text consumed once, owner thread only. */
const char *arm_namespace_error(ArmNamespace *);
int arm_namespace_state(const ArmNamespace *,uint32_t);
#endif
