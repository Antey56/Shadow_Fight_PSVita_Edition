#ifndef SF_ARM_ARENA_VITA_H
#define SF_ARM_ARENA_VITA_H
#include "arm_arena.h"
#include <psp2/types.h>
/* Only valid user-callable imports from the matching kubridge version are
   accepted. These must never be raw kernel addresses. No discovery/installation
   or calls occur until an explicit caller supplies the imports and opens. */
typedef struct {
    int (*protect)(void *,SceSize,uint32_t);
    void (*flush)(const void *,SceSize);
} SfArenaKuImports;
typedef struct {
    ArmArena arena;
    SceUID block;
    void *base;
    size_t capacity;
    SfArenaKuImports imports;
} SfVitaArena;
/* Fresh object only. Backs the arena with ordinary user RW memory; sync uses
   kubridge to mark each published image RWX and flush caches. Not per-segment
   W^X. Requires device validation with the exact plugin before game mapping. */
int sf_vita_arena_open(SfVitaArena *,size_t,const SfArenaKuImports *);
/* Caller must release all mapped images after module shutdown before close. */
int sf_vita_arena_close(SfVitaArena *);
#endif
