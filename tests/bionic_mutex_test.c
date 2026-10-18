#include "bionic_mutex.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
static int32_t shared;
static unsigned count;
static void *work(void *unused){
    (void)unused;for(unsigned i=0;i<5000;i++){
        assert(!sf_bionic_mutex_lock(&shared));count++;assert(!sf_bionic_mutex_unlock(&shared));
    }return 0;
}
int main(void){
    pthread_t workers[8];for(unsigned i=0;i<8;i++)assert(!pthread_create(&workers[i],0,work,0));
    for(unsigned i=0;i<8;i++)assert(!pthread_join(workers[i],0));
    assert(count==40000);
    assert(!sf_bionic_mutex_destroy(&shared));assert(sf_bionic_mutex_lock(&shared)==22);
    int32_t recursive=0x4000,errorcheck=0x8000;
    assert(!sf_bionic_mutex_lock(&recursive));assert(!sf_bionic_mutex_lock(&recursive));
    assert(!sf_bionic_mutex_unlock(&recursive));assert(!sf_bionic_mutex_unlock(&recursive));assert(!sf_bionic_mutex_destroy(&recursive));
    assert(!sf_bionic_mutex_lock(&errorcheck));assert(sf_bionic_mutex_lock(&errorcheck)==35);
    assert(sf_bionic_mutex_trylock(&errorcheck)==16);assert(sf_bionic_mutex_destroy(&errorcheck)==16);
    assert(!sf_bionic_mutex_unlock(&errorcheck));assert(!sf_bionic_mutex_destroy(&errorcheck));
    int32_t attr=0,control=0;assert(!sf_bionic_mutexattr_init(&attr));assert(!sf_bionic_mutexattr_settype(&attr,1));
    assert(!sf_bionic_mutex_init(&control,&attr));assert(sf_bionic_mutex_init(&control,&attr)==16);
    assert(!sf_bionic_mutex_lock(&control));assert(!sf_bionic_mutex_trylock(&control));
    assert(!sf_bionic_mutex_unlock(&control));assert(!sf_bionic_mutex_unlock(&control));
    assert(!sf_bionic_mutexattr_destroy(&attr));assert(sf_bionic_mutexattr_settype(&attr,0)==22);
    assert(!sf_bionic_mutex_destroy(&control));assert(!sf_bionic_mutex_init(&control,0));assert(!sf_bionic_mutex_destroy(&control));
    int32_t invalid=123;assert(sf_bionic_mutex_lock(&invalid)==22);
    puts("ARM32 mutex storage/recursion/concurrent updates PASS");return 0;
}
