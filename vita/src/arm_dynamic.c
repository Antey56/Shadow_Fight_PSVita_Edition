#include "arm_dynamic.h"
#include <string.h>
typedef struct {uint32_t offset,va,filesz,memsz;} Load;
typedef struct {uint32_t tag,value;int seen;} Tag;
static uint16_t r16(const unsigned char *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t r32(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int fits(uint64_t off,uint64_t bytes,size_t size){return off<=size&&bytes<=size-off;}
static int translate(const Load *loads,size_t n,uint32_t va,uint32_t bytes,uint32_t *result){
    unsigned matches=0;
    for(size_t i=0;i<n;i++)if(va>=loads[i].va&&(uint64_t)va+bytes<=(uint64_t)loads[i].va+loads[i].filesz){
        *result=loads[i].offset+(va-loads[i].va);matches++;
    }
    return matches==1;
}
static int target_valid(const Load *loads,size_t n,uint32_t va){
    if(va&3)return 0;
    for(size_t i=0;i<n;i++)if(va>=loads[i].va&&(uint64_t)va+4<=(uint64_t)loads[i].va+loads[i].memsz)return 1;
    return 0;
}
static int unsupported(uint32_t t){
    return t==7||t==8||t==35||t==36||t==37||
        (t>=0x6000000f&&t<=0x60000012)||t==0x6fffe000||t==0x6fffe001;
}
int arm_dynamic_read(const unsigned char *b,size_t size,ArmRawRel *out,size_t cap,ArmDynamicInfo *info){
    if(!info)return -1;
    memset(info,0,sizeof(*info));
    if(!b||(!out&&cap)||size>UINT32_MAX||size<52)return -1;
    if(memcmp(b,"\177ELF\1\1\1",7)||r16(b+16)!=3||r16(b+18)!=40||r32(b+20)!=1||r16(b+40)!=52)return -2;
    uint32_t po=r32(b+28);unsigned pn=r16(b+44);
    if(r16(b+42)!=32||!pn||pn>128||!fits(po,(uint64_t)pn*32,size))return -3;
    Load loads[128];size_t nl=0;uint32_t dynoff=0,dynva=0,dynsize=0;unsigned ndyn=0;
    ArmDynamicInfo result={0};result.load_min=UINT32_MAX;
    for(unsigned i=0;i<pn;i++){
        const unsigned char *p=b+po+i*32;uint32_t type=r32(p),off=r32(p+4),va=r32(p+8),fs=r32(p+16),ms=r32(p+20),align=r32(p+28);
        if(!fits(off,fs,size))return -4;
        if(type==2){ndyn++;dynoff=off;dynva=va;dynsize=fs;}
        if(type!=1||!ms)continue;
        if(fs>ms||(uint64_t)va+ms>UINT32_MAX)return -4;
        if(align>1&&((align&(align-1))||va%align!=off%align))return -4;
        for(size_t j=0;j<nl;j++)if((uint64_t)va+ms>loads[j].va&&(uint64_t)loads[j].va+loads[j].memsz>va)return -4;
        loads[nl++]=(Load){off,va,fs,ms};
        if(va<result.load_min)result.load_min=va;
        if(va+ms>result.load_end)result.load_end=va+ms;
    }
    uint32_t translated;
    if(!nl||ndyn!=1||!dynsize||dynsize%8||!translate(loads,nl,dynva,dynsize,&translated)||translated!=dynoff)return -5;
    /* Supported singleton tags; DT_NEEDED may repeat and is not resolved here. */
    Tag tags[]={{5,0,0},{10,0,0},{6,0,0},{11,0,0},{17,0,0},{18,0,0},{19,0,0},{23,0,0},{2,0,0},{20,0,0}};
    int terminated=0;
    for(uint32_t pos=0;pos<dynsize;pos+=8){
        uint32_t t=r32(b+dynoff+pos),v=r32(b+dynoff+pos+4);
        if(!t){terminated=1;break;}
        if(unsupported(t))return -6;
        for(size_t j=0;j<sizeof(tags)/sizeof(tags[0]);j++)if(tags[j].tag==t){
            if(tags[j].seen)return -5;
            tags[j].seen=1;tags[j].value=v;
        }
    }
    if(!terminated)return -5;
    for(unsigned j=0;j<4;j++)if(!tags[j].seen)return -5;
    if(tags[3].value!=16||!tags[1].value||!translate(loads,nl,tags[0].value,tags[1].value,&result.strtab_file_offset)||!translate(loads,nl,tags[2].value,16,&translated))return -5;
    result.strtab_size=tags[1].value;result.symtab_vaddr=tags[2].value;
    uint32_t table_va[2]={0},table_size[2]={0};
    if(tags[4].seen||tags[5].seen){
        if(!tags[4].seen||!tags[5].seen||!tags[6].seen||tags[6].value!=8||tags[5].value%8)return -7;
        table_va[0]=tags[4].value;table_size[0]=tags[5].value;
    }
    if(tags[7].seen||tags[8].seen){
        if(!tags[7].seen||!tags[8].seen||!tags[9].seen||tags[9].value!=17||tags[8].value%8)return -7;
        table_va[1]=tags[7].value;table_size[1]=tags[8].value;
    }
    if(table_size[0]&&table_size[1]&&(uint64_t)table_va[0]+table_size[0]>table_va[1]&&(uint64_t)table_va[1]+table_size[1]>table_va[0])return -7;
    result.relocation_count=(size_t)table_size[0]/8+(size_t)table_size[1]/8;
    if(out&&cap<result.relocation_count)return -8;
    size_t written=0;
    for(unsigned j=0;j<2;j++){
        if(!table_size[j])continue;
        uint32_t off;if(!translate(loads,nl,table_va[j],table_size[j],&off))return -7;
        for(uint32_t pos=0;pos<table_size[j];pos+=8){
            uint32_t target=r32(b+off+pos),ri=r32(b+off+pos+4),type=ri&255,index=ri>>8;
            if(type&& !target_valid(loads,nl,target))return -9;
            if(type!=0&&type!=2&&type!=21&&type!=22&&type!=23)return -6;
            if(type==23&&index)return -6;
            if(index){
                uint64_t sym=(uint64_t)result.symtab_vaddr+(uint64_t)index*16;
                uint32_t so;if(sym>UINT32_MAX||!translate(loads,nl,(uint32_t)sym,16,&so))return -10;
                uint32_t name=r32(b+so);if(name>=result.strtab_size)return -10;
                size_t start=result.strtab_file_offset+name,end=result.strtab_file_offset+result.strtab_size;
                if(!memchr(b+start,0,end-start))return -10;
            }
            if(out)out[written]=(ArmRawRel){target,type,index};
            written++;
        }
    }
    *info=result;return 0;
}
