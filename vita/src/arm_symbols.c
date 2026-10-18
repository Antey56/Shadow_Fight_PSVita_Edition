#include "arm_symbols.h"
#include "arm_dynamic.h"
#include <string.h>
static uint16_t r16(const unsigned char *p){return p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const unsigned char *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int offset(const unsigned char *b,uint32_t va,uint64_t bytes,uint32_t *out){
    uint32_t po=r32(b+28);unsigned count=r16(b+44);
    for(unsigned i=0;i<count;i++){
        const unsigned char *p=b+po+32*i;
        if(r32(p)==1&&va>=r32(p+8)&&(uint64_t)va+bytes<=(uint64_t)r32(p+8)+r32(p+16)){
            *out=r32(p+4)+va-r32(p+8);return 1;
        }
    }
    return 0;
}
static int defined(const unsigned char *b,uint32_t va,uint32_t bytes,int executable){
    uint32_t po=r32(b+28);unsigned count=r16(b+44);
    for(unsigned i=0;i<count;i++){
        const unsigned char *p=b+po+32*i;
        uint32_t size=executable?r32(p+16):r32(p+20);
        if(r32(p)==1&&(!executable||(r32(p+24)&1))&&va>=r32(p+8)&&
           (uint64_t)va+bytes<=(uint64_t)r32(p+8)+size)return 1;
    }
    return 0;
}
static int version_defined(const unsigned char *b,const ArmDynamicInfo *info,uint32_t va,uint32_t count,unsigned wanted){
    if(!va||!count||count>1024)return -1;
    int found=0;
    for(uint32_t i=0;i<count;i++){
        uint32_t off;if(!offset(b,va,20,&off))return -1;
        const unsigned char *d=b+off;unsigned n=r16(d+6),index=r16(d+4);
        uint32_t aux=r32(d+12),next=r32(d+16);
        if(r16(d)!=1||!n||n>64||aux<20||(uint64_t)va+aux>UINT32_MAX)return -1;
        uint32_t cursor=va+aux;
        for(unsigned j=0;j<n;j++){
            uint32_t ao;if(!offset(b,cursor,8,&ao))return -1;
            uint32_t name=r32(b+ao),step=r32(b+ao+4);
            if(name>=info->strtab_size||!memchr(b+info->strtab_file_offset+name,0,info->strtab_size-name))return -1;
            if(j+1<n){if(step<8||(uint64_t)cursor+step>UINT32_MAX)return -1;cursor+=step;}
            else if(step)return -1;
        }
        if(index==wanted){if(found)return -1;found=1;}
        if(i+1<count){if(next<20||(uint64_t)va+next>UINT32_MAX)return -1;va+=next;}
        else if(next)return -1;
    }
    return found;
}
int arm_symbol_find(const unsigned char *b,size_t size,const char *name,uint32_t bias,ArmExport *out){
    if(!out)return -60;
    memset(out,0,sizeof(*out));
    if(!name||!name[0])return -60;
    ArmDynamicInfo info;int rc=arm_dynamic_read(b,size,0,0,&info);if(rc)return rc;
    uint32_t po=r32(b+28),hash=0,versions=0,verdef=0,vercount=0;int seen_hash=0,seen_versions=0,seen_def=0,seen_count=0;
    for(unsigned i=0;i<r16(b+44);i++){
        const unsigned char *p=b+po+32*i;if(r32(p)!=2)continue;
        for(uint32_t j=0;j<r32(p+16);j+=8){
            uint32_t tag=r32(b+r32(p+4)+j),value=r32(b+r32(p+4)+j+4);if(!tag)break;
            if(tag==4){if(seen_hash++)return -61;hash=value;}
            if(tag==0x6ffffff0){if(seen_versions++)return -61;versions=value;}
            if(tag==0x6ffffffc){if(seen_def++)return -61;verdef=value;}
            if(tag==0x6ffffffd){if(seen_count++)return -61;vercount=value;}
        }
    }
    uint32_t off;if(!seen_hash||!offset(b,hash,8,&off))return -61;
    uint32_t buckets=r32(b+off),count=r32(b+off+4);
    if(!buckets||!count||count>1000000||!offset(b,hash,8ull+4ull*((uint64_t)buckets+count),&off))return -61;
    uint32_t symbols;if(!offset(b,info.symtab_vaddr,16ull*count,&symbols))return -62;
    uint32_t veroff=0;if(seen_versions&&!offset(b,versions,2ull*count,&veroff))return -62;
    ArmExport best={0};int found=0;
    for(uint32_t i=1;i<count;i++){
        const unsigned char *s=b+symbols+16*i;uint32_t n=r32(s),value=r32(s+4),bytes=r32(s+8);
        unsigned bind=s[12]>>4,type=s[12]&15,visibility=s[13]&3;uint16_t section=r16(s+14);
        if(n>=info.strtab_size)return -63;
        const char *candidate=(const char *)(b+info.strtab_file_offset+n);
        if(!memchr(candidate,0,info.strtab_size-n))return -63;
        if(strcmp(name,candidate)||!section||(bind!=1&&bind!=2)||(visibility!=0&&visibility!=3))continue;
        if(seen_versions){
            unsigned version=r16(b+veroff+2*i);
            if(!version||(version&0x8000))continue; /* local/nondefault version */
            if(version>1&&version_defined(b,&info,verdef,vercount,version)!=1)return -66;
        }
        if(type>2||(section>=0xff00&&section!=0xfff1))return -64;
        if(type==2&&!(value&1)&&(value&3))return -65;
        if(section!=0xfff1&&!defined(b,type==2?(value&~1u):value,bytes?bytes:type==2?2:0,type==2))return -65;
        if(section!=0xfff1)value+=bias; /* ARM ELF32 load bias is modular. */
        if(found&&best.binding==1&&bind==1)return -64; /* Unhandled duplicate/version policy. */
        if(!found||bind==1){best=(ArmExport){value,bytes,bind,type,section==0xfff1};found=1;}
    }
    if(found)*out=best;
    return found;
}
