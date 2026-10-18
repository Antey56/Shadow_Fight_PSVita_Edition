#ifndef SF_ARM_ARENA_H
#define SF_ARM_ARENA_H
#include "arm_exec.h"
#define ARM_ARENA_SLOTS 64
/* Borrowed backing allocation. Callbacks must copy/synchronize without executing
   module code. All operations are confined to one loader owner thread. */
typedef struct {
    void *context;
    int (*write)(void *,size_t,const unsigned char *,size_t);
    int (*sync)(void *,size_t,size_t);
} ArmArenaStorage;
typedef struct {
    uint32_t offset,span,requested,generation;
    unsigned active,phase;
} ArmArenaSlot;
typedef struct {
    uint32_t base,capacity;
    ArmArenaStorage storage;
    ArmArenaSlot slots[ARM_ARENA_SLOTS];
} ArmArena;
/* Fresh arena only; never reinitialize with outstanding cookies or images.
   Does not allocate, protect or free platform backing memory. */
int arm_arena_init(ArmArena *,uint32_t,size_t,const ArmArenaStorage *);
ArmExecBackend arm_arena_backend(ArmArena *);
size_t arm_arena_used(const ArmArena *);
#endif
