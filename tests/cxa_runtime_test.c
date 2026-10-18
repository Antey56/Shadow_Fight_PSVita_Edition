#include "libmain_runtime.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static unsigned log_values[128],count;
static void record(void *value){log_values[count++]=(unsigned)(uintptr_t)value;}
static void nested(void *unused){(void)unused;record((void *)8);assert(!sf_runtime_atexit(record,(void *)9,(void *)2));}
int main(void){
    assert(sf_runtime_atexit(0,0,0)<0);
    assert(!sf_runtime_atexit(record,(void *)1,(void *)1));
    assert(!sf_runtime_atexit(record,(void *)2,(void *)2));
    assert(!sf_runtime_atexit(record,(void *)3,(void *)1));
    sf_runtime_finalize((void *)1);assert(count==2&&log_values[0]==3&&log_values[1]==1);
    sf_runtime_finalize((void *)1);assert(count==2);
    assert(!sf_runtime_atexit(nested,0,(void *)2));sf_runtime_finalize(0);
    assert(count==5&&log_values[2]==8&&log_values[3]==9&&log_values[4]==2);
    for(unsigned i=0;i<64;i++)assert(!sf_runtime_atexit(record,(void *)(uintptr_t)i,(void *)3));
    assert(sf_runtime_atexit(record,0,0)<0);sf_runtime_finalize((void *)3);assert(count==69);
    assert(!sf_runtime_atexit(record,(void *)99,0));sf_runtime_finalize(0);assert(log_values[69]==99);
    puts("DSO destructor ownership/reentrancy PASS");return 0;
}
