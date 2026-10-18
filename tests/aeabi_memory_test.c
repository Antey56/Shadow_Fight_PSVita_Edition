#include "aeabi_memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
    void (*copies[])(void *,const void *,size_t)={sf_aeabi_memcpy,sf_aeabi_memcpy4,sf_aeabi_memcpy8};
    void (*moves[])(void *,const void *,size_t)={sf_aeabi_memmove,sf_aeabi_memmove4,sf_aeabi_memmove8};
    void (*sets[])(void *,size_t,int)={sf_aeabi_memset,sf_aeabi_memset4,sf_aeabi_memset8};
    void (*clears[])(void *,size_t)={sf_aeabi_memclr,sf_aeabi_memclr4,sf_aeabi_memclr8};
    uint64_t backing[16],source[16];unsigned char *a=(void *)backing,*b=(void *)source;
    for(size_t v=0;v<3;v++)for(size_t n=0;n<=63;n++){
        for(size_t i=0;i<128;i++){a[i]=0xa5;b[i]=(unsigned char)i;}
        copies[v](a+8,b+8,n);assert(!memcmp(a+8,b+8,n));assert(a[7]==0xa5&&a[8+n]==0xa5);
        sets[v](a+8,n,0x1ab);for(size_t i=0;i<n;i++)assert(a[8+i]==0xab);
        assert(a[7]==0xa5&&a[8+n]==0xa5);
        clears[v](a+8,n);for(size_t i=0;i<n;i++)assert(!a[8+i]);assert(a[8+n]==0xa5);
        for(size_t i=0;i<128;i++)a[i]=b[i]=(unsigned char)i;
        moves[v](a+8,a,n);memmove(b+8,b,n);assert(!memcmp(a,b,128));
        moves[v](a,a+8,n);memmove(b,b+8,n);assert(!memcmp(a,b,128));
        moves[v](a,a,n);assert(!memcmp(a,b,128));
    }
    assert(sf_aeabi_memory_catalog(0,0)==-22);
    puts("RTABI memory host semantics PASS; ARM assembly needs target validation");return 0;
}
