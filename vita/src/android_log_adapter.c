#include "android_log_adapter.h"
#include <stdio.h>
#include <stdint.h>
static SfLogSink current_sink;
static void *sink_context;
static int minimum=SF_LOG_VERBOSE;
static int stderr_sink(const SfLogRecord *r,void *unused){
    (void)unused;
    return fprintf(stderr,"[%d/%d] %s: %s%s\n",r->buffer,r->priority,r->tag,r->text,r->truncated?" [truncated]":"")>=0;
}
int sf_log_configure(SfLogSink sink,void *context,int min){
    if(min<SF_LOG_VERBOSE||min>SF_LOG_SILENT||(!sink&&context))return -22;
    current_sink=sink;sink_context=context;minimum=min;return 0;
}
static int valid(int buffer,int priority){
    /* Text buffers only: MAIN, RADIO, SYSTEM, CRASH. Binary event/security
       buffers and newer logd/property APIs are not implemented. */
    if((buffer!=0&&buffer!=1&&buffer!=3&&buffer!=4)||priority<2||priority>7)return -22;
    return priority<minimum?-1:0; /* Android -EPERM, not host errno encoding */
}
static int copy_bounded(char *dst,size_t cap,const char *src){
    size_t i=0;
    if(!src)src="";
    while(i+1<cap&&src[i]){dst[i]=src[i];i++;}
    dst[i]=0;return src[i]!=0;
}
static int emit(int buffer,int priority,const char *tag,const char *text,int truncated){
    char bounded_tag[96],bounded_text[1024];
    truncated|=copy_bounded(bounded_tag,sizeof bounded_tag,tag);
    truncated|=copy_bounded(bounded_text,sizeof bounded_text,text);
    SfLogRecord record={buffer,priority,truncated,bounded_tag,bounded_text};
    SfLogSink sink=current_sink?current_sink:stderr_sink;
    return sink(&record,sink_context)>0?1:-5; /* local output failure: Android -EIO */
}
static int format(int buffer,int priority,const char *tag,const char *fmt,va_list ap){
    int rc=valid(buffer,priority);if(rc)return rc;
    if(!fmt)return -22;
    char text[1024];va_list copy;va_copy(copy,ap);
    int n=vsnprintf(text,sizeof text,fmt,copy);va_end(copy);
    if(n<0)return -22;
    return emit(buffer,priority,tag,text,(size_t)n>=sizeof text);
}
SF_ANDROID_ABI int __android_log_write(int priority,const char *tag,const char *text){
    int rc=valid(0,priority);return rc?rc:emit(0,priority,tag,text,0);
}
SF_ANDROID_ABI int __android_log_buf_write(int buffer,int priority,const char *tag,const char *text){
    int rc=valid(buffer,priority);return rc?rc:emit(buffer,priority,tag,text,0);
}
SF_ANDROID_ABI int __android_log_print(int priority,const char *tag,const char *fmt,...){
    va_list ap;va_start(ap,fmt);int rc=format(0,priority,tag,fmt,ap);va_end(ap);return rc;
}
SF_ANDROID_ABI int __android_log_buf_print(int buffer,int priority,const char *tag,const char *fmt,...){
    va_list ap;va_start(ap,fmt);int rc=format(buffer,priority,tag,fmt,ap);va_end(ap);return rc;
}
SF_ANDROID_ABI int __android_log_vprint(int priority,const char *tag,const char *fmt,va_list ap){
    return format(0,priority,tag,fmt,ap);
}
int sf_log_target_catalog(ArmScopeEntry *out,size_t cap){
#if defined(SF2_VITA_TARGET) && defined(__arm__)
    if(!out||cap<5||sizeof(uintptr_t)!=4)return -22;
#define ENTRY(name) (ArmScopeEntry){"host:liblog",#name,(uint32_t)(uintptr_t)&name,2,1}
    out[0]=ENTRY(__android_log_write);out[1]=ENTRY(__android_log_buf_write);
    out[2]=ENTRY(__android_log_print);out[3]=ENTRY(__android_log_buf_print);
    out[4]=ENTRY(__android_log_vprint);
#undef ENTRY
    return 5;
#else
    (void)out;(void)cap;return -22;
#endif
}
