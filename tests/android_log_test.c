#include "android_log_adapter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int calls,fail_output,buffer,priority,truncated;
static char tag[96],message[1024];
static int capture(const SfLogRecord *r,void *ctx){
    assert(ctx==&calls);calls++;buffer=r->buffer;priority=r->priority;truncated=r->truncated;
    strcpy(tag,r->tag);strcpy(message,r->text);return fail_output?0:1;
}
static int forward(const char *fmt,...){
    char again[128];va_list ap;va_start(ap,fmt);
    int rc=__android_log_vprint(4,"forward",fmt,ap);
    vsnprintf(again,sizeof again,fmt,ap);va_end(ap);
    assert(!strcmp(again,message));return rc;
}
int main(void){
    assert(sf_log_configure(capture,&calls,2)==0);
    assert(__android_log_write(4,"tag","100% literal")==1&&calls==1);
    assert(!strcmp(message,"100% literal")&&!strcmp(tag,"tag")&&!truncated);
    assert(__android_log_print(5,"printf","i=%d d=%.2f",7,1.25)==1);
    assert(!strcmp(message,"i=7 d=1.25")&&priority==5&&buffer==0);
    assert(forward("v=%d",9)==1);
    assert(__android_log_buf_print(3,4,"sys","%s","system")==1&&buffer==3);
    assert(__android_log_buf_write(4,6,"crash","message")==1&&buffer==4);
    assert(sf_log_configure(capture,&calls,6)==0);int before=calls;
    assert(__android_log_print(4,"drop","filtered")==-1&&calls==before);
    assert(sf_log_configure(capture,&calls,2)==0);
    char long_text[2048];memset(long_text,'x',sizeof long_text-1);long_text[sizeof long_text-1]=0;
    assert(__android_log_write(4,"long",long_text)==1&&strlen(message)==1023&&truncated);
    assert(__android_log_print(4,"long","%s",long_text)==1&&strlen(message)==1023&&truncated);
    assert(__android_log_write(4,long_text,"short")==1&&strlen(tag)==95&&truncated);
    assert(__android_log_write(4,0,0)==1&&!strcmp(tag,"")&&!strcmp(message,""));
    before=calls;assert(__android_log_buf_write(2,4,"event","unsupported")==-22&&calls==before);
    assert(__android_log_write(1,"bad","priority")==-22&&calls==before);
    assert(__android_log_print(4,"bad",0)==-22&&calls==before);
    fail_output=1;assert(__android_log_write(4,"io","failure")==-5);fail_output=0;
    assert(sf_log_configure(capture,&calls,9)==-22);
    ArmScopeEntry entries[5];assert(sf_log_target_catalog(entries,5)==-22); /* host pointers refused */
    assert(sf_log_configure(0,0,2)==0);
    puts("Android text logging adapter PASS");return 0;
}
