#include "bionic_once.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
static volatile int32_t once;
static unsigned calls,payload;
static void initialize(void){calls++;payload=0x12345678;}
static void worker(void){
    for(unsigned i=0;i<1000;i++){
        assert(!sf_bionic_pthread_once(&once,initialize));assert(payload==0x12345678);
    }
}
#ifdef _WIN32
static DWORD WINAPI entry(LPVOID unused){(void)unused;worker();return 0;}
#else
static void *entry(void *unused){(void)unused;worker();return 0;}
#endif
int main(void){
    assert(sf_bionic_pthread_once(0,initialize)==22);
    assert(sf_bionic_pthread_once(&once,0)==22);
    uint32_t storage[2];assert(sf_bionic_pthread_once((volatile int32_t *)((unsigned char *)storage+1),initialize)==22);
    int32_t invalid=9;assert(sf_bionic_pthread_once(&invalid,initialize)==22&&invalid==9);
#ifdef _WIN32
    HANDLE threads[8];for(unsigned i=0;i<8;i++){threads[i]=CreateThread(0,0,entry,0,0,0);assert(threads[i]);}
    WaitForMultipleObjects(8,threads,TRUE,INFINITE);for(unsigned i=0;i<8;i++)CloseHandle(threads[i]);
#else
    pthread_t threads[8];for(unsigned i=0;i<8;i++)assert(!pthread_create(&threads[i],0,entry,0));
    for(unsigned i=0;i<8;i++)assert(!pthread_join(threads[i],0));
#endif
    assert(calls==1&&once==2);puts("Android ARM32 once: concurrent publication PASS");return 0;
}
