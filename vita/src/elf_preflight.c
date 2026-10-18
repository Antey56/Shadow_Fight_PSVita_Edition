#include "elf_preflight.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
static uint16_t le16(const unsigned char *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t le32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int fits(uint64_t offset,uint64_t size,uint64_t total){return offset<=total&&size<=total-offset;}
int sf2_elf_inspect(const char *path,ElfPreflight *out){
    if(!path||!out)return -1;
    memset(out,0,sizeof(*out));
    FILE *f=fopen(path,"rb");if(!f)return -1;
    unsigned char h[52],ph[32];int rc=-2;
    if(fseek(f,0,SEEK_END)!=0)goto done;
    long end=ftell(f);if(end<52)goto done;
    uint64_t total=(uint64_t)end;
    if(fseek(f,0,SEEK_SET)!=0||fread(h,1,sizeof h,f)!=sizeof h)goto done;
    if(memcmp(h,"\177ELF",4))goto done;
    rc=-3;
    if(h[4]!=1||h[5]!=1||h[6]!=1||le16(h+18)!=40)goto done;
    rc=-4;
    if(le16(h+16)!=3||le32(h+20)!=1||le16(h+40)!=52)goto done;
    uint32_t off=le32(h+28);uint16_t stride=le16(h+42),count=le16(h+44);
    rc=-5;
    if(stride!=32||!count||count>128||!fits(off,(uint64_t)stride*count,total))goto done;
    out->flags=le32(h+36);out->load_min=UINT32_MAX;
    rc=-6;if(out->flags&0x400)goto done;
    for(unsigned i=0;i<count;i++){
        uint64_t position=(uint64_t)off+(uint64_t)i*stride;
        rc=-5;if(position>LONG_MAX||fseek(f,(long)position,SEEK_SET)||fread(ph,1,32,f)!=32)goto done;
        uint32_t type=le32(ph),p=le32(ph+4),va=le32(ph+8),fs=le32(ph+16),ms=le32(ph+20),flags=le32(ph+24),align=le32(ph+28);
        rc=-7;if(!fits(p,fs,total))goto done;
        if(type==2)out->has_dynamic=1;
        if(type==7)out->has_tls=1;
        if(type!=1)continue;
        if(fs>ms||(uint64_t)va+ms>UINT32_MAX)goto done;
        if(align>1&&((align&(align-1))||((va%align)!=(p%align))))goto done;
        if(!ms)continue;
        if(va<out->load_min)out->load_min=va;
        if(va+ms>out->load_max)out->load_max=va+ms;
        out->load_count++;
        if((flags&3)==3)out->writable_executable=1;
    }
    rc=-8;if(!out->load_count||!out->has_dynamic)goto done;
    rc=0;
done:
    fclose(f);if(rc)memset(out,0,sizeof(*out));return rc;
}
const char *sf2_elf_error(int code){
    switch(code){
    case 0:return "structurally valid ARM32 shared object; NOT execution-certified";
    case -1:return "invalid argument or file open failed";
    case -2:return "truncated file or invalid ELF magic";
    case -3:return "requires ELF32 little-endian EM_ARM version 1";
    case -4:return "requires ET_DYN with valid ELF header";
    case -5:return "invalid or unsupported program header table";
    case -6:return "hard-float ABI flag rejected";
    case -7:return "segment bounds, alignment or memory size invalid";
    case -8:return "missing loadable segments or dynamic segment";
    default:return "unknown error";
    }
}
