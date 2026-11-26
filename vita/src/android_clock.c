#include "android_clock.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
static const SfClockSource *source;
int sf_android_clock_read(const SfClockSource *clock,int id,SfAndroidTimespec32 *out,int *error){
    if(!error)return -1;
    *error=0;
    if(!out){*error=14;return -1;}
    if(!clock||!clock->read||id<0||id>=32||!(clock->mask&(1u<<(unsigned)id))){*error=22;return -1;}
    SfClockSample sample={0,0};int rc=clock->read(clock->context,id,&sample);
    if(rc){*error=rc>0?rc:5;return -1;}
    if(sample.nanoseconds<0||sample.nanoseconds>=1000000000){*error=5;return -1;}
    if(sample.seconds<INT32_MIN||sample.seconds>INT32_MAX){*error=75;return -1;}
    SfAndroidTimespec32 result={(int32_t)sample.seconds,sample.nanoseconds};*out=result;return 0;
}
void sf_android_clock_bind(const SfClockSource *clock){source=clock;}
SF_ANDROID_ABI int sf_android_clock_gettime(int id,SfAndroidTimespec32 *out){
    int error=0,rc=sf_android_clock_read(source,id,out,&error);if(rc)errno=error;return rc;
}
