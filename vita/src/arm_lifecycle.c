#include "arm_lifecycle.h"
#include "arm_dynamic.h"
#include <stdlib.h>
#include <string.h>
static uint16_t r16(const unsigned char *p){return p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const unsigned char *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int compare(const void *a,const void *b){
    uint32_t x=((const ArmRawRel *)a)->target_vaddr,y=((const ArmRawRel *)b)->target_vaddr;
    return x>y?1:x<y?-1:0;
}
static const ArmRawRel *relocation(const ArmRawRel *rels,size_t count,uint32_t slot){
    size_t lo=0,hi=count;
    while(lo<hi){size_t mid=lo+(hi-lo)/2;if(rels[mid].target_vaddr<slot)lo=mid+1;else hi=mid;}
    return lo<count&&rels[lo].target_vaddr==slot?&rels[lo]:0;
}
static int offset(const unsigned char *file,uint32_t va,uint32_t bytes,uint32_t *out){
    uint32_t ph=r32(file+28);unsigned count=r16(file+44);
    for(unsigned i=0;i<count;i++){
        const unsigned char *p=file+ph+32*i;
        if(r32(p)==1&&va>=r32(p+8)&&(uint64_t)va+bytes<=(uint64_t)r32(p+8)+r32(p+16)){
            *out=r32(p+4)+va-r32(p+8);return 1;
        }
    }return 0;
}
static int target(const unsigned char *file,const ArmLinkedImage *image,uint32_t pointer,uint32_t *out){
    uint32_t raw=pointer&~1u;unsigned width=pointer&1?2:4;
    if(!pointer||(!(pointer&1)&&(raw&3)))return -104;
    uint32_t ph=r32(file+28);unsigned count=r16(file+44);
    for(unsigned i=0;i<count;i++){
        const unsigned char *p=file+ph+32*i;
        if(r32(p)==1&&(r32(p+24)&1)&&raw>=r32(p+8)&&
           (uint64_t)raw+width<=(uint64_t)r32(p+8)+r32(p+16)){
            *out=image->bias+pointer;return 0;
        }
    }return -104;
}
static int array(const unsigned char *file,const ArmLinkedImage *image,uint32_t address,uint32_t bytes,
                  const ArmRawRel *rels,size_t n,uint32_t *out,size_t *count){
    if(address%4||bytes%4||bytes/4>ARM_LIFECYCLE_MAX)return -102;
    if(!bytes)return 0;
    uint32_t off;if(!offset(file,address,bytes,&off))return -102;
    for(uint32_t i=0;i<bytes/4;i++){
        uint32_t slot=address+i*4,raw=r32(file+off+i*4);
        const ArmRawRel *rel=relocation(rels,n,slot);
        if(!rel){if(raw==0||raw==UINT32_MAX)continue;return -103;}
        if(rel->type!=23||rel->symbol_index)return -103;
        if(slot<image->vaddr_min||(uint64_t)slot-image->vaddr_min+4>image->size)return -102;
        if(r32(image->bytes+slot-image->vaddr_min)!=image->bias+raw)return -103;
        if(*count==ARM_LIFECYCLE_MAX)return -105;
        int rc=target(file,image,raw,&out[*count]);if(rc)return rc;
        (*count)++;
    }return 0;
}
int arm_lifecycle_plan(const unsigned char *file,size_t size,const ArmLinkedImage *image,ArmLifecyclePlan *out){
    if(!out)return -100;
    memset(out,0,sizeof(*out));
    ArmDynamicInfo info;int rc=arm_dynamic_read(file,size,0,0,&info);
    if(rc)return rc;
    if(!image||!image->bytes||image->vaddr_min!=info.load_min||
       image->size!=(size_t)info.load_end-info.load_min)return -100;
    uint32_t base=image->bias+info.load_min;
    if((uint64_t)base+image->size>UINT32_MAX)return -100;
    if(info.relocation_count>SIZE_MAX/sizeof(ArmRawRel))return -106;
    ArmRawRel *rels=calloc(info.relocation_count?info.relocation_count:1,sizeof(*rels));
    if(!rels)return -106;
    rc=arm_dynamic_read(file,size,rels,info.relocation_count,&info);if(rc)goto done;
    size_t n=0;
    for(size_t i=0;i<info.relocation_count;i++)if(rels[i].type)rels[n++]=rels[i];
    qsort(rels,n,sizeof(*rels),compare);
    const uint32_t tags[8]={12,13,25,27,26,28,32,33};uint32_t values[8]={0};unsigned seen[8]={0};
    uint32_t ph=r32(file+28);unsigned count=r16(file+44);
    for(unsigned i=0;i<count;i++){
        const unsigned char *p=file+ph+32*i;if(r32(p)!=2)continue;
        for(uint32_t j=0;j<r32(p+16);j+=8){
            uint32_t t=r32(file+r32(p+4)+j),v=r32(file+r32(p+4)+j+4);if(!t)break;
            for(unsigned k=0;k<8;k++)if(t==tags[k]){
                if(seen[k]++){rc=-101;goto done;}values[k]=v;
            }
        }
    }
    if(seen[2]!=seen[3]||seen[4]!=seen[5]||seen[6]!=seen[7]||values[6]||values[7]){rc=-101;goto done;}
    if(values[0]){rc=target(file,image,values[0],&out->init[out->init_count]);if(rc)goto done;out->init_count++;}
    rc=array(file,image,values[2],values[3],rels,n,out->init,&out->init_count);if(rc)goto done;
    rc=array(file,image,values[4],values[5],rels,n,out->fini,&out->fini_count);if(rc)goto done;
    for(size_t i=0;i<out->fini_count/2;i++){
        uint32_t tmp=out->fini[i];out->fini[i]=out->fini[out->fini_count-i-1];out->fini[out->fini_count-i-1]=tmp;
    }
    if(values[1]){
        if(out->fini_count==ARM_LIFECYCLE_MAX){rc=-105;goto done;}
        rc=target(file,image,values[1],&out->fini[out->fini_count]);if(rc)goto done;out->fini_count++;
    }
    out->base=base;out->image_size=image->size;out->validated=1;rc=0;
done:
    free(rels);if(rc)memset(out,0,sizeof(*out));return rc;
}
