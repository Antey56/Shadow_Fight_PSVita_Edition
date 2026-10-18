#include "arm_exec.h"
#include "arm_dynamic.h"
#include <string.h>
static uint16_t r16(const unsigned char *p){return p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const unsigned char *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
void arm_exec_release(ArmExecImage *image){
    if(image){if(image->ready)image->backend.release(image->backend.context,image->cookie);memset(image,0,sizeof(*image));}
}
int arm_exec_map(const unsigned char *file,size_t size,size_t limit,ArmResolve resolve,void *ctx,
                 const ArmExecBackend *backend,ArmExecImage *out,ArmLinkError *error){
    if(error)memset(error,0,sizeof(*error));
    if(!out)return -30;
    memset(out,0,sizeof(*out));
    if(!backend||!backend->reserve||!backend->publish||!backend->sync||!backend->release)return -30;
    ArmDynamicInfo info;int rc=arm_dynamic_read(file,size,0,0,&info);if(rc)return rc;
    size_t span=(size_t)info.load_end-info.load_min,alignment=4096;
    if(!span||span>limit)return -31;
    uint32_t po=r32(file+28);unsigned count=r16(file+44),nx=0;
    for(unsigned i=0;i<count;i++){
        const unsigned char *p=file+po+32*i;uint32_t type=r32(p),flags=r32(p+24),a=r32(p+28);
        if(type==7)return -32;
        if(type==2){
            uint32_t off=r32(p+4),bytes=r32(p+16);
            for(uint32_t j=0;j+8<=bytes;j+=8){
                uint32_t tag=r32(file+off+j),value=r32(file+off+j+4);if(!tag)break;
                if(value&&(tag==12||tag==25||tag==32))return -32; /* INIT, INIT_ARRAY, PREINIT_ARRAY */
            }
        }
        if(type!=1)continue;
        if(a>alignment)alignment=a;
        if((flags&3)==3)return -32;
        if(flags&1){if(!r32(p+16)||++nx>ARM_EXEC_MAX_RANGES)return -32;}
    }
    if(!nx||alignment>limit)return -32;
    uintptr_t cookie=0;uint32_t address=0;
    rc=backend->reserve(backend->context,span,alignment,&address,&cookie);if(rc)return rc;
    ArmLinkedImage linked={0};ArmExecImage next={0};next.backend=*backend;next.cookie=cookie;
    if(address%alignment||(uint64_t)address+span>UINT32_MAX){rc=-33;goto fail;}
    rc=arm_link_image(file,size,address,limit,resolve,ctx,&linked,error);if(rc)goto fail;
    rc=backend->publish(backend->context,cookie,linked.bytes,linked.size);if(rc)goto fail;
    rc=backend->sync(backend->context,cookie,linked.size);if(rc)goto fail;
    next.base=address;next.bias=linked.bias;next.size=span;
    for(unsigned i=0;i<count;i++){
        const unsigned char *p=file+po+32*i;
        if(r32(p)==1&&(r32(p+24)&1)){
            next.code[next.code_count].begin=linked.bias+r32(p+8);
            next.code[next.code_count++].end=linked.bias+r32(p+8)+r32(p+16);
        }
    }
    next.ready=1;*out=next;arm_link_dispose(&linked);return 0;
fail:
    arm_link_dispose(&linked);backend->release(backend->context,cookie);return rc;
}
int arm_exec_entry(const ArmExecImage *image,uint32_t pointer,uint32_t *address){
    if(address)*address=0;
    if(!image||!image->ready||!address)return -34;
    uint32_t raw=pointer&~1u,min=image->base-image->bias;
    if(raw<min)return -34;
    uint64_t runtime=(uint64_t)image->base+(raw-min);
    unsigned width=(pointer&1)?2:4;
    if(!(pointer&1)&&(raw&3))return -34;
    if(runtime>UINT32_MAX)return -34;
    for(unsigned i=0;i<image->code_count;i++)if(runtime>=image->code[i].begin&&runtime+width<=image->code[i].end){
        *address=(uint32_t)runtime|(pointer&1);return 0;
    }
    return -34;
}
