#include "bionic_memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
    unsigned char *p=sf_bionic_calloc(256,1);assert(p);
    for(unsigned i=0;i<256;i++)assert(p[i]==0);
    for(unsigned i=0;i<256;i++)p[i]=(unsigned char)i;
    p=sf_bionic_realloc(p,512);assert(p);
    for(unsigned i=0;i<256;i++)assert(p[i]==(unsigned char)i);
    assert(sf_bionic_memchr(p,255,256)==p+255);
    assert(sf_bionic_memchr(p,255,255)==0);
    unsigned char *q=sf_bionic_malloc(256);assert(q);
    assert(sf_bionic_memcpy(q,p,256)==q);assert(sf_bionic_memcmp(p,q,256)==0);
    q[128]=0;assert(sf_bionic_memcmp(p,q,256)>0);
    assert(sf_bionic_memset(q,0xab,256)==q);assert(q[255]==0xab);
    sf_bionic_free(p);sf_bionic_free(q);sf_bionic_free(0);
    assert(!sf_bionic_calloc(SIZE_MAX,2));
    char overlap[16]="0123456789";
    sf_bionic_memmove(overlap+2,overlap,8);assert(!memcmp(overlap,"0101234567",10));
    sf_bionic_memmove(overlap,overlap+2,8);assert(!memcmp(overlap,"01234567",8));
    char text[64];assert(sf_bionic_strcpy(text,"abca")==text);
    assert(sf_bionic_strlen(text)==4);assert(sf_bionic_strchr(text,'a')==text);
    assert(sf_bionic_strrchr(text,'a')==text+3);assert(sf_bionic_strchr(text,0)==text+4);
    assert(sf_bionic_strstr(text,"bc")==text+1);assert(!sf_bionic_strstr(text,"xy"));
    assert(sf_bionic_strcmp("\x80","\x7f")>0);assert(sf_bionic_strncmp("abc","abd",2)==0);
    sf_bionic_strcat(text,"XY");sf_bionic_strncat(text,"1234",2);assert(!strcmp(text,"abcaXY12"));
    memset(text,0xff,sizeof text);sf_bionic_strncpy(text,"x",4);
    assert(text[0]=='x'&&text[1]==0&&text[3]==0&&(unsigned char)text[4]==255);
    char *copy=sf_bionic_strdup("asset-name");assert(copy&&!strcmp(copy,"asset-name"));sf_bionic_free(copy);
    assert(sf_bionic_memory_catalog(0,0)==-22);
    puts("memory/string adapters PASS");return 0;
}
