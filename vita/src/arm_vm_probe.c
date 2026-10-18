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

#ifdef SF2_LIBMAIN_ENTRY_PROBE
#include "libmain_probe_image.h"
#include "libmain_runtime.h"
#include "bionic_memory.h"
#include "aeabi_memory.h"
#include "jni_vm.h"
typedef struct {ArmSymbolScope scope;VmAllocation *allocation;} MainResolve;
static int main_resolver(const char *name,unsigned bind,unsigned type,uint32_t *value,void *context){
    MainResolve *state=context;ArmExport own;
    int found=arm_symbol_find(sf_libmain_probe_image,sizeof sf_libmain_probe_image,name,
                             (uint32_t)(uintptr_t)state->allocation->base,&own);
    if(found<0)return -1;
    if(found){if(type&&own.type&&type!=own.type)return -1;*value=own.value;return 1;}
    return arm_scope_resolve(name,bind,type,value,&state->scope);
}
int sf_libmain_entry_probe(int *result,uint32_t *version,uint32_t *load,uint32_t *unload,SfArmProbeTrace reporter){
    *result=sf_libmain_runtime_prepare();if(*result)return 0;
    ArmScopeEntry entries[54];
    if(sf_bionic_memory_catalog(entries,20)!=20||sf_aeabi_memory_catalog(entries+20,12)!=12||
       sf_libmain_runtime_catalog(entries+32,22)!=22){*result=-72;return 0;}
    VmAllocation allocation={-1,0,reporter};MainResolve state;state.allocation=&allocation;
    if(arm_scope_init(&state.scope,entries,54)){*result=-73;return 0;}
    ArmExecBackend backend={&allocation,reserve,publish,sync_code,release};ArmExecImage image;ArmLinkError error;
    *result=arm_exec_map(sf_libmain_probe_image,sizeof sf_libmain_probe_image,1024*1024,main_resolver,&state,&backend,&image,&error);
    if(*result){if(reporter)reporter(error.missing_symbol,*result,0);return 0;}
    /* exidx belongs to this exact image; no host unwind or C++ compatibility is certified. */
    uint32_t exidx=0,count=0;const unsigned char *b=sf_libmain_probe_image;
    uint32_t po=(uint32_t)b[28]|((uint32_t)b[29]<<8)|((uint32_t)b[30]<<16)|((uint32_t)b[31]<<24);
    for(unsigned i=0;i<(unsigned)(b[44]|(b[45]<<8));i++){
        const unsigned char *p=b+po+32*i;uint32_t words[8];
        for(unsigned j=0;j<8;j++)words[j]=(uint32_t)p[j*4]|((uint32_t)p[j*4+1]<<8)|((uint32_t)p[j*4+2]<<16)|((uint32_t)p[j*4+3]<<24);
        if(words[0]==0x70000001){exidx=image.bias+words[2];count=words[4]/8;}
    }
    sf_libmain_runtime_bind(image.base,image.size,exidx,count);
    static const NativeDeclaration declarations[]={
        {"com/unity3d/player/NativeLoader","load","(Ljava/lang/String;)Z"},
        {"com/unity3d/player/NativeLoader","unload","()Z"}};
    SfJniCodeRange ranges[ARM_EXEC_MAX_RANGES];
    for(unsigned i=0;i<image.code_count;i++){ranges[i].begin=image.code[i].begin;ranges[i].end=image.code[i].end;}
    SfJavaVM *vm=sf_jni_vm_create("ux0:data/sf2vita/assets",4096);SfJniEnv *env=0;int ok=0;
    if(!vm){*result=-74;goto done;}
    if((*vm)->GetEnv(vm,(void **)&env,0x10006)||sf_jni_bridge_configure_natives(env,declarations,2,ranges,image.code_count)){*result=-75;goto vm_done;}
    ArmExport entry;uint32_t address;
    if(arm_symbol_find(b,sizeof sf_libmain_probe_image,"JNI_OnLoad",0,&entry)!=1||entry.value!=0x10e5||arm_exec_entry(&image,entry.value,&address)){*result=-76;goto vm_done;}
    typedef int32_t (SF_ANDROID_ABI *OnLoad)(SfJavaVM *,void *);
    if(reporter)reporter("libmain.before_JNI_OnLoad",0,address);
    *version=(uint32_t)((OnLoad)(uintptr_t)address)(vm,0);
    if(reporter)reporter("libmain.returned_JNI_OnLoad",(int)*version,address);
    *load=(uint32_t)sf_jni_bridge_registered(env,declarations[0].class_name,"load","(Ljava/lang/String;)Z");
    *unload=(uint32_t)sf_jni_bridge_registered(env,declarations[0].class_name,"unload","()Z");
    ok=*version==0x10006&&*load==image.bias+0x1145&&*unload==image.bias+0x11e1&&!(*env)->ExceptionCheck(env);
    *result=ok?0:-77;
vm_done:
    sf_jni_vm_release(vm);
done:
    sf_runtime_finalize(0);sf_libmain_runtime_bind(0,0,0,0);arm_exec_release(&image);return ok;
}
#else
int sf_libmain_entry_probe(int *result,uint32_t *version,uint32_t *load,uint32_t *unload,SfArmProbeTrace reporter){
    (void)version;(void)load;(void)unload;(void)reporter;*result=-78;return 0;
}
#endif
