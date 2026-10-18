#include "arm_arena_vita.h"
#include <psp2/kernel/sysmem.h>
#include <string.h>
/* Protection bits verified against bythos14/kubridge exceptions_mprotect. */
#define SF_KU_RWX (0x40u|0x20u|0x10u)
static int write_image(void *ctx,size_t offset,const unsigned char *data,size_t size){
    SfVitaArena *v=ctx;
    if(offset>v->capacity||size>v->capacity-offset)return -80;
    memcpy((unsigned char *)v->base+offset,data,size);return 0;
}
static int sync_image(void *ctx,size_t offset,size_t size){
    SfVitaArena *v=ctx;size_t rounded=(size+4095u)&~(size_t)4095u;
    if(offset%4096||offset>v->capacity||rounded>v->capacity-offset)return -80;
    void *address=(unsigned char *)v->base+offset;
    int rc=v->imports.protect(address,(SceSize)rounded,SF_KU_RWX);
    if(rc<0)return rc;
    v->imports.flush(address,(SceSize)rounded);return 0;
}
int sf_vita_arena_open(SfVitaArena *v,size_t capacity,const SfArenaKuImports *imports){
    if(!v)return -80;
    memset(v,0,sizeof(*v));v->block=-1;
    if(!imports||!imports->protect||!imports->flush||!capacity||capacity%4096||capacity>64u*1024u*1024u)return -80;
    v->imports=*imports;
    v->block=sceKernelAllocMemBlock("sf2-module-arena",SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,(SceSize)capacity,0);
    if(v->block<0)return v->block;
    int rc=sceKernelGetMemBlockBase(v->block,&v->base);
    if(rc<0)goto fail;
    v->capacity=capacity;
    ArmArenaStorage storage={v,write_image,sync_image};
    rc=arm_arena_init(&v->arena,(uint32_t)(uintptr_t)v->base,capacity,&storage);
    if(rc<0)goto fail;
    return 0;
fail:
    sceKernelFreeMemBlock(v->block);v->block=-1;v->base=0;v->capacity=0;return rc;
}
int sf_vita_arena_close(SfVitaArena *v){
    if(!v)return -80;
    if(arm_arena_used(&v->arena))return -81;
    if(v->block<0)return 0;
    int rc=sceKernelFreeMemBlock(v->block);
    if(rc>=0){memset(&v->arena,0,sizeof v->arena);v->block=-1;v->base=0;v->capacity=0;}
    return rc;
}
