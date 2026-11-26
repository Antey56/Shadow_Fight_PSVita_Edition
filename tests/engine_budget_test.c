#include "engine_budget.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
int main(void){
    int threshold=7;const unsigned mb=1024*1024;
    assert(!sf_engine_ram_threshold(119*mb,32*mb,80*mb,&threshold)&&threshold==87*(int)mb);
    assert(!sf_engine_ram_threshold(55*mb,32*mb,16*mb,&threshold)&&threshold==23*(int)mb);
    assert(sf_engine_ram_threshold(47*mb,32*mb,16*mb,&threshold)==-121&&threshold==0);
    assert(!sf_engine_ram_threshold(48*mb,32*mb,16*mb,&threshold)&&threshold==16*(int)mb);
    assert(sf_engine_ram_threshold(1,4096,0,&threshold)==-121);
    assert(sf_engine_ram_threshold(119*mb,0,0,&threshold)==-120);
    assert(sf_engine_ram_threshold(119*mb,4097,0,&threshold)==-120);
    assert(sf_engine_ram_threshold(119*mb,4096,1,&threshold)==-120);
    assert(sf_engine_ram_threshold(UINT_MAX,4096,0,&threshold)==-120);
    assert(sf_engine_ram_threshold(119*mb,4096,0,0)==-120);
    puts("Engine RAM cap, pending arena headroom and underflow checks PASS");return 0;
}
