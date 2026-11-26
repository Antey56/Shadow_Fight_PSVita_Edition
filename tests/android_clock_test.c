#include "android_clock.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
typedef struct {SfClockSample sample;int rc,id,calls;} Provider;
static int read_clock(void *ctx,int id,SfClockSample *sample){
    Provider *p=ctx;p->id=id;p->calls++;*sample=p->sample;return p->rc;
}
int main(void){
    assert(sizeof(SfAndroidTimespec32)==8);assert(offsetof(SfAndroidTimespec32,nanoseconds)==4);
    Provider p={{123,456},0,0,0};SfClockSource source={&p,(1u<<1)|(1u<<7),read_clock};
    SfAndroidTimespec32 out={11,22};int error=0;
    assert(!sf_android_clock_read(&source,1,&out,&error)&&!error&&out.seconds==123&&out.nanoseconds==456&&p.id==1);
    p.sample.seconds=456;assert(!sf_android_clock_read(&source,7,&out,&error)&&p.id==7&&out.seconds==456);
    source.mask=1u<<1;out.seconds=11;out.nanoseconds=22;int calls=p.calls;
    assert(sf_android_clock_read(&source,7,&out,&error)==-1&&error==22&&p.calls==calls&&out.seconds==11&&out.nanoseconds==22);
    assert(sf_android_clock_read(&source,-1,&out,&error)==-1&&error==22);
    assert(sf_android_clock_read(&source,32,&out,&error)==-1&&error==22);
    assert(sf_android_clock_read(&source,1,0,&error)==-1&&error==14);
    p.sample.seconds=(int64_t)INT32_MAX+1;
    assert(sf_android_clock_read(&source,1,&out,&error)==-1&&error==75&&out.seconds==11);
    p.sample.seconds=(int64_t)INT32_MIN-1;
    assert(sf_android_clock_read(&source,1,&out,&error)==-1&&error==75);
    p.sample.seconds=-1;p.sample.nanoseconds=999999999;
    assert(!sf_android_clock_read(&source,1,&out,&error)&&out.seconds==-1&&out.nanoseconds==999999999);
    p.sample.nanoseconds=1000000000;
    assert(sf_android_clock_read(&source,1,&out,&error)==-1&&error==5);
    p.sample.nanoseconds=-1;
    assert(sf_android_clock_read(&source,1,&out,&error)==-1&&error==5);
    p.rc=95;assert(sf_android_clock_read(&source,1,&out,&error)==-1&&error==95);
    p.rc=-1;assert(sf_android_clock_read(&source,1,&out,&error)==-1&&error==5);
    sf_android_clock_bind(0);errno=0;assert(sf_android_clock_gettime(1,&out)==-1&&errno==22);
    p.rc=0;p.sample.seconds=1;p.sample.nanoseconds=2;sf_android_clock_bind(&source);
    errno=17;assert(!sf_android_clock_gettime(1,&out)&&errno==17&&out.seconds==1&&out.nanoseconds==2);
    sf_android_clock_bind(0);puts("Android time32 layout, explicit clock IDs, overflow/error and no boottime alias PASS");return 0;
}
