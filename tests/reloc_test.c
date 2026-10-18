#include "arm_reloc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t word(const unsigned char *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int main(void){
    unsigned char image[24]={4,0,0,0,8,0,0,0,12,0,0,0,99,0,0,0};size_t fail;
    ArmReloc r[]={{0,23,0,0,0},{4,2,0x2000,1,1},{8,21,0x3000,1,0},{12,22,0x4000,1,1}};
    assert(arm_reloc_apply(image,sizeof image,0x1000,r,4,&fail)==0);
    assert(word(image)==0x1004);assert(word(image+4)==0x2009);
    assert(word(image+8)==0x300c);assert(word(image+12)==0x4001);
    unsigned char before[24];memcpy(before,image,24);
    ArmReloc bad[]={{0,23,0,0,0},{4,160,0,0,0}};
    assert(arm_reloc_apply(image,24,0,bad,2,&fail)==-3&&fail==1);assert(!memcmp(image,before,24));
    bad[1]=(ArmReloc){4,2,0,0,0};assert(arm_reloc_apply(image,24,0,bad,2,&fail)==-5);assert(!memcmp(image,before,24));
    bad[1]=(ArmReloc){23,23,0,0,0};assert(arm_reloc_apply(image,24,0,bad,2,&fail)==-4);
    bad[1]=(ArmReloc){0,23,0,0,0};assert(arm_reloc_apply(image,24,0,bad,2,&fail)==-7);assert(!memcmp(image,before,24));
    bad[1]=(ArmReloc){4,23,4,0,0};assert(arm_reloc_apply(image,24,0,bad,2,&fail)==-6);
    ArmReloc wrap={16,2,UINT32_MAX,1,0};image[16]=2;
    assert(arm_reloc_apply(image,24,0,&wrap,1,&fail)==0&&word(image+16)==1);
    ArmReloc none={UINT32_MAX,0,0,0,0};assert(arm_reloc_apply(image,24,0,&none,1,&fail)==0);
    puts("ARM REL relocation tests PASS");return 0;
}
