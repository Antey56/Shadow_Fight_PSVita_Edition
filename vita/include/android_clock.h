#ifndef SF_ANDROID_CLOCK_H
#define SF_ANDROID_CLOCK_H
#include <stdint.h>
#include "android_abi.h"
/* Android ARM32/time32 layout, never alias a native time_t/timespec. */
typedef struct {int32_t seconds,nanoseconds;} SfAndroidTimespec32;
typedef struct {int64_t seconds;int32_t nanoseconds;} SfClockSample;
/* Immutable backend. Mask bits are Android clock IDs, not host IDs.
   Callback returns zero or a positive ANDROID errno. Clock 1 excludes suspend;
   clock 7 includes it. Never claim both unless backend semantics are verified. */
typedef struct {void *context;uint32_t mask;int (*read)(void *,int,SfClockSample *);} SfClockSource;
int sf_android_clock_read(const SfClockSource *,int,SfAndroidTimespec32 *,int *);
/* Bind before starting engine workers; source/context must remain immutable and
   live until process exit. No provider is installed by default. */
void sf_android_clock_bind(const SfClockSource *);
SF_ANDROID_ABI int sf_android_clock_gettime(int,SfAndroidTimespec32 *);
#endif
