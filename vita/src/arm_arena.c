#include "arm_arena.h"
#include <string.h>
#define PAGE 4096u
static ArmArenaSlot *lookup(ArmArena *a,uintptr_t cookie){
    if(!a||cookie>UINT32_MAX)return 0;
    unsigned index=(unsigned)cookie&255u;uint32_t generation=(uint32_t)cookie>>8;
    if(!index||index>ARM_ARENA_SLOTS||!generation)return 0;
    ArmArenaSlot *s=&a->slots[index-1];
    return s->active&&s->generation==generation?s:0;
}
static int reserve(void *ctx,size_t bytes,size_t alignment,uint32_t *base,uintptr_t *cookie){
    ArmArena *a=ctx;
    if(base)*base=0;
    if(cookie)*cookie=0;
    if(!a||!base||!cookie||!a->capacity||!bytes||bytes>a->capacity||
       !alignment||(alignment&(alignment-1))||alignment>0x40000000u)return -60;
    if(alignment<PAGE)alignment=PAGE;
    uint64_t span=((uint64_t)bytes+PAGE-1)&~(uint64_t)(PAGE-1);
    size_t free_slot=ARM_ARENA_SLOTS;
    for(size_t i=0;i<ARM_ARENA_SLOTS;i++)if(!a->slots[i].active&&a->slots[i].generation<0xffffffu){free_slot=i;break;}
    if(free_slot==ARM_ARENA_SLOTS)return -61;
    uint64_t candidate=a->base,limit=(uint64_t)a->base+a->capacity;
    for(unsigned pass=0;pass<=ARM_ARENA_SLOTS;pass++){
        candidate=(candidate+alignment-1)&~(uint64_t)(alignment-1);
        if(candidate+span>limit)return -62;
        uint64_t next=candidate;
        for(size_t i=0;i<ARM_ARENA_SLOTS;i++){
            const ArmArenaSlot *s=&a->slots[i];if(!s->active)continue;
            uint64_t lo=(uint64_t)a->base+s->offset,hi=lo+s->span;
            if(candidate<hi&&candidate+span>lo&&hi>next)next=hi;
        }
        if(next!=candidate){candidate=next;continue;}
        ArmArenaSlot *s=&a->slots[free_slot];
        s->offset=(uint32_t)(candidate-a->base);s->span=(uint32_t)span;
        s->requested=(uint32_t)bytes;s->generation++;s->active=1;s->phase=0;
        *base=(uint32_t)candidate;*cookie=((uintptr_t)s->generation<<8)|(free_slot+1);return 0;
    }
    return -62;
}
static int publish(void *ctx,uintptr_t cookie,const unsigned char *bytes,size_t count){
    ArmArena *a=ctx;ArmArenaSlot *s=lookup(a,cookie);
    if(!s||s->phase||!bytes||count!=s->requested)return -63;
    int rc=a->storage.write(a->storage.context,s->offset,bytes,count);
    if(!rc)s->phase=1;
    return rc;
}
static int sync_code(void *ctx,uintptr_t cookie,size_t count){
    ArmArena *a=ctx;ArmArenaSlot *s=lookup(a,cookie);
    if(!s||s->phase!=1||count!=s->requested)return -64;
    int rc=a->storage.sync(a->storage.context,s->offset,count);
    if(!rc)s->phase=2;
    return rc;
}
static void release(void *ctx,uintptr_t cookie){
    ArmArenaSlot *s=lookup(ctx,cookie);
    if(s){s->active=0;s->phase=0;s->requested=0;s->span=0;s->offset=0;}
}
int arm_arena_init(ArmArena *a,uint32_t base,size_t capacity,const ArmArenaStorage *storage){
    if(!a)return -60;
    memset(a,0,sizeof(*a));
    if(!storage||!storage->write||!storage->sync||base%PAGE||!base||!capacity||
       capacity%PAGE||(uint64_t)base+capacity>UINT32_MAX)return -60;
    a->base=base;a->capacity=(uint32_t)capacity;a->storage=*storage;return 0;
}
ArmExecBackend arm_arena_backend(ArmArena *a){
    ArmExecBackend backend={a,reserve,publish,sync_code,release};return backend;
}
size_t arm_arena_used(const ArmArena *a){
    size_t n=0;if(a)for(size_t i=0;i<ARM_ARENA_SLOTS;i++)if(a->slots[i].active)n+=a->slots[i].span;
    return n;
}
