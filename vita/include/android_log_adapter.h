#ifndef SF_ANDROID_LOG_ADAPTER_H
#define SF_ANDROID_LOG_ADAPTER_H
#include <stdarg.h>
#include "arm_scope.h"
#include "android_abi.h"
enum { SF_LOG_VERBOSE=2,SF_LOG_DEBUG=3,SF_LOG_INFO=4,SF_LOG_WARN=5,SF_LOG_ERROR=6,SF_LOG_FATAL=7,SF_LOG_SILENT=8 };
typedef struct {int buffer,priority,truncated;const char *tag,*text;} SfLogRecord;
/* Synchronous sink: return >0 on success, <=0 on failure. Pointers expire on
   return: copy if needed. Sink must be thread-safe and must not call this logger.
   Configure only at startup or with all logging threads externally stopped. */
typedef int (*SfLogSink)(const SfLogRecord *,void *);
int sf_log_configure(SfLogSink,void *,int minimum_priority);
/* Native target catalog is available only in a Vita ARM build. This function
   never truncates 64-bit host pointers into purported ARM addresses. */
int sf_log_target_catalog(ArmScopeEntry *,size_t capacity);
SF_ANDROID_ABI int __android_log_write(int,const char *,const char *);
SF_ANDROID_ABI int __android_log_buf_write(int,int,const char *,const char *);
SF_ANDROID_ABI int __android_log_print(int,const char *,const char *,...);
SF_ANDROID_ABI int __android_log_buf_print(int,int,const char *,const char *,...);
SF_ANDROID_ABI int __android_log_vprint(int,const char *,const char *,va_list);
#endif
