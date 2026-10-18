#include "arm_link.h"
#include "arm_dynamic.h"
#include "arm_reloc.h"
#include <stdlib.h>
#include <string.h>
static uint16_t r16(const unsigned char *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t r32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int sym_offset(const unsigned char *b,uint64_t va,uint32_t *off){
    uint32_t po=r32(b+28);unsigned pn=r16(b+44);
    for(unsigned i=0;i<pn;i++){
        const unsigned char *p=b+po+32*i;
        if(r32(p)==1&&va>=r32(p+8)&&va+16<=(uint64_t)r32(p+8)+r32(p+16)){
            *off=r32(p+4)+(uint32_t)(va-r32(p+8));return 1;
        }
    }
    return 0;
}
static int defined_range(const unsigned char *b,uint32_t va,uint32_t bytes){
    uint32_t po=r32(b+28);unsigned pn=r16(b+44);
    for(unsigned i=0;i<pn;i++){
        const unsigned char *p=b+po+32*i;
        if(r32(p)==1&&va>=r32(p+8)&&(uint64_t)va+bytes<=(uint64_t)r32(p+8)+r32(p+20))return 1;
    }
    return 0;
}
void arm_link_dispose(ArmLinkedImage *out){if(out){free(out->bytes);memset(out,0,sizeof(*out));}}
int arm_link_image(const unsigned char *b,size_t size,uint32_t start,size_t limit,ArmResolve resolve,void *ctx,ArmLinkedImage *out,ArmLinkError *error){
    if(error){memset(error,0,sizeof(*error));error->relocation_index=(size_t)-1;}
    if(!out)return -1;
    memset(out,0,sizeof(*out));
    ArmDynamicInfo info;int rc=arm_dynamic_read(b,size,0,0,&info);
    ArmRawRel *raw=0;ArmReloc *ready=0;unsigned char *image=0;size_t index=(size_t)-1;
    if(rc)goto fail;
    rc=-15;if(r32(b+36)&0x400)goto fail;
    size_t span=(size_t)info.load_end-info.load_min;
    rc=-11;
    if(!span||span>limit||(uint64_t)start+span>UINT32_MAX)goto fail;
    if(info.relocation_count>SIZE_MAX/sizeof(*raw)||info.relocation_count>SIZE_MAX/sizeof(*ready))goto fail;
    rc=-12;
    raw=calloc(info.relocation_count?info.relocation_count:1,sizeof(*raw));
    ready=calloc(info.relocation_count?info.relocation_count:1,sizeof(*ready));
    image=calloc(span,1);if(!raw||!ready||!image)goto fail;
    rc=arm_dynamic_read(b,size,raw,info.relocation_count,&info);if(rc)goto fail;
    uint32_t bias=start-info.load_min;
    uint32_t po=r32(b+28);unsigned pn=r16(b+44);
    rc=-13;
    for(unsigned i=0;i<pn;i++){
        const unsigned char *p=b+po+i*32;if(r32(p)!=1||!r32(p+20))continue;
        uint32_t va=r32(p+8),off=r32(p+4),fs=r32(p+16),alignment=r32(p+28);
        if(alignment>1&&bias%alignment)goto fail;
        memcpy(image+(va-info.load_min),b+off,fs);
    }
    for(index=0;index<info.relocation_count;index++){
        ArmRawRel *r=&raw[index];ArmReloc *dst=&ready[index];
        dst->type=r->type;dst->offset=r->type?r->target_vaddr-info.load_min:0;
        if(!r->type||r->type==23)continue;
        if(!r->symbol_index){dst->symbol_resolved=1;continue;}
        uint32_t off;rc=-14;
        if(!sym_offset(b,(uint64_t)info.symtab_vaddr+16ull*r->symbol_index,&off))goto fail;
        const unsigned char *s=b+off;unsigned bind=s[12]>>4,type=s[12]&15,visibility=s[13]&3;
        uint16_t section=r16(s+14);uint32_t value=r32(s+4),name=r32(s);
        if(type>2||bind>2||(section>=0xff00&&section!=0xfff1)||(!section&&!bind))goto fail;
        if(section&&section!=0xfff1&&!defined_range(b,type==2?(value&~1u):value,r32(s+8)))goto fail;
        const char *symbol=(const char *)(b+info.strtab_file_offset+name);
        int found=0;
        if(section&&(bind==0||visibility!=0)){
            value=section==0xfff1?value:bias+value;found=1;
        }else if(resolve){found=resolve(symbol,bind,type,&value,ctx);}
        if(found<0){rc=-21;goto fail;}
        if(!found){
            if(!section&&bind==2)value=0;
            else{
                rc=-20;
                if(error){strncpy(error->missing_symbol,symbol,sizeof(error->missing_symbol)-1);}
                goto fail;
            }
        }
        dst->symbol_resolved=1;dst->thumb=(type==2&&(value&1))!=0;
        dst->symbol_value=dst->thumb?(value&~1u):value;
    }
    rc=arm_reloc_apply(image,span,bias,ready,info.relocation_count,&index);if(rc)goto fail;
    out->bytes=image;out->size=span;out->vaddr_min=info.load_min;out->bias=bias;
    free(raw);free(ready);return 0;
fail:
    if(error){error->code=rc;error->relocation_index=index;}
    free(raw);free(ready);free(image);return rc;
}
