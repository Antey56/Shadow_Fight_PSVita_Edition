#include "arm_vm_probe.h"
#include "arm_exec.h"
#include "arm_symbols.h"
#include "android_abi.h"
#include <psp2/kernel/sysmem.h>
#include <stdint.h>
#include <string.h>
#include "arm_smoke_image.h"
typedef struct {SceUID block;void *base;SfArmProbeTrace trace;} VmAllocation;
static void trace(VmAllocation *a,const char *name,int rc){if(a->trace)a->trace(name,rc,(uint32_t)(uintptr_t)a->base);}
static int reserve(void *context,size_t bytes,size_t alignment,uint32_t *base,uintptr_t *cookie){
    VmAllocation *a=context;
    if(bytes>16u*1024u*1024u||alignment>4096)return -40;
    size_t rounded=16u*1024u*1024u; /* Known VM-domain allocation size used by so-loader. */
    a->block=sceKernelAllocMemBlockForVM("sf2-authored-arm",rounded);trace(a,"reserve",a->block);if(a->block<0)return a->block;
    int rc=sceKernelGetMemBlockBase(a->block,&a->base);
    if(rc<0){sceKernelFreeMemBlock(a->block);a->block=-1;return rc;}
    rc=sceKernelOpenVMDomain();trace(a,"open_before_write",rc);
    if(rc<0){sceKernelFreeMemBlock(a->block);a->block=-1;return rc;}
    *base=(uint32_t)(uintptr_t)a->base;*cookie=(uintptr_t)a->block;return 0;
}
static int publish(void *context,uintptr_t cookie,const unsigned char *bytes,size_t count){
    VmAllocation *a=context;if((SceUID)cookie!=a->block)return -41;
    trace(a,"before_copy",0);memcpy(a->base,bytes,count);trace(a,"after_copy",0);return 0;
}
static int sync_code(void *context,uintptr_t cookie,size_t count){
    VmAllocation *a=context;trace(a,"before_cache_sync",0);
    int rc=sceKernelSyncVMDomain((SceUID)cookie,a->base,count);trace(a,"after_cache_sync",rc);return rc;
}
static void release(void *context,uintptr_t cookie){
    VmAllocation *a=context;sceKernelFreeMemBlock((SceUID)cookie);a->block=-1;a->base=0;
}
static SF_ANDROID_ABI float fixture_add(float a,float b){return a+b;}
static int resolver(const char *name,unsigned binding,unsigned type,uint32_t *value,void *context){
    (void)binding;(void)context;
    if((type==0||type==2)&&!strcmp(name,"sf_fixture_add")){*value=(uint32_t)(uintptr_t)&fixture_add;return 1;}
    return 0;
}
/* Executes authored embedded fixture only; no game ELF input is accepted. */
int sf_arm_vm_probe(int *link_rc,uint32_t *entry,float *first,float *second,char *missing,size_t capacity,SfArmProbeTrace reporter){
    if(missing&&capacity)missing[0]=0;
    VmAllocation allocation={-1,0,reporter};ArmExecBackend backend={&allocation,reserve,publish,sync_code,release};
    ArmExecImage image;ArmLinkError error;
    int rc=arm_exec_map(sf_arm_smoke_image,sizeof sf_arm_smoke_image,1024*1024,resolver,0,&backend,&image,&error);
    *link_rc=rc;if(rc){
        if(missing&&capacity){strncpy(missing,error.missing_symbol,capacity-1);missing[capacity-1]=0;}
        return 0;
    }
    ArmExport symbol;rc=arm_symbol_find(sf_arm_smoke_image,sizeof sf_arm_smoke_image,"sf_fixture_entry",0,&symbol);
    if(rc!=1||symbol.type!=2||symbol.value!=SF_ARM_SMOKE_ENTRY){
        *link_rc=rc<0?rc:-42;arm_exec_release(&image);return 0;
    }
    uint32_t address;rc=arm_exec_entry(&image,symbol.value,&address);*link_rc=rc;
    if(rc){arm_exec_release(&image);return 0;}
    *entry=address;
    typedef float (SF_ANDROID_ABI *Entry)(float,float);
    Entry call=(Entry)(uintptr_t)address;
    if(reporter)reporter("before_entry",0,address);
    *first=call(1.25f,2.5f);
    if(reporter)reporter("returned_first",0,address);
    *second=call(-2.0f,0.5f);
    if(reporter)reporter("returned_second",0,address);
    int ok=*first==11.75f&&*second==7.5f;
    arm_exec_release(&image);return ok;
}
