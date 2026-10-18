#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <vitaGL.h>
#include <stdio.h>
#include "touch.h"
#include "diagnostic_ui.h"
#include "arm_reloc.h"
#include "native_registry.h"
int _newlib_heap_size_user=96*1024*1024;
/* Diagnostic renderer only. kubridge is a memory/cache bridge, NOT graphics. */
static FILE *logfile;
static void probe(void){}
static int selftest(void){
    unsigned char b[4]={4,0,0,0};ArmReloc rel={0,23,0,0,0};size_t failed;
    NativeRegistry registry={0};
    return arm_reloc_apply(b,4,0x1000,&rel,1,&failed)==0&&b[0]==4&&b[1]==16&&
        native_bind(&registry,"diagnostic","probe","()V",probe)==0&&
        native_lookup(&registry,"diagnostic","probe","()V")==probe;
}
static void sink(const TouchEvent *e,void *unused){
    (void)unused;
    if(logfile){fprintf(logfile,"touch action=%d index=%d count=%d time=%llu\n",e->action,e->index,e->count,(unsigned long long)e->time_us);
        for(int i=0;i<e->count;i++)fprintf(logfile,"  id=%d x=%.1f y=%.1f\n",e->points[i].id,e->points[i].x,e->points[i].y);
        fflush(logfile);}
    /* TODO: marshal complete event to verified game JNI/native input entrypoint.
       Android action integer = action | (index << 8). This struct is NOT JNI. */
}
int main(void){
    logfile=fopen("ux0:data/sf2vita-input.log","w");
    int core_ok=selftest();
    if(logfile){fprintf(logfile,"SF2 diagnostic 0.2; Unity NOT started; core check=%s\n",core_ok?"PASS":"FAIL");fflush(logfile);}
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    if(!vglInit(8*1024*1024)){if(logfile)fclose(logfile);return 1;}
    TouchState state={0};SceCtrlData pad={0};
    for(;;){
        if(sceCtrlPeekBufferPositive(0,&pad,1)<=0){touch_cancel(&state,0,sink,0);sceKernelDelayThread(16000);continue;}
        if(pad.buttons&SCE_CTRL_START)break;
        unsigned b=0;
#define MAP(v,a) if(pad.buttons&(v))b|=(a)
        MAP(SCE_CTRL_LEFT,B_LEFT);MAP(SCE_CTRL_RIGHT,B_RIGHT);MAP(SCE_CTRL_UP,B_UP);MAP(SCE_CTRL_DOWN,B_DOWN);
        MAP(SCE_CTRL_CROSS,B_CROSS);MAP(SCE_CTRL_SQUARE,B_SQUARE);MAP(SCE_CTRL_TRIANGLE,B_TRIANGLE);MAP(SCE_CTRL_CIRCLE,B_CIRCLE);
        touch_step(&state,b,(pad.lx-128)/128.0f,(pad.ly-128)/128.0f,pad.timeStamp,sink,0);
        diagnostic_ui(&state,logfile!=0,core_ok);vglSwapBuffers(GL_FALSE);
    }
    touch_cancel(&state,pad.timeStamp,sink,0);if(logfile)fclose(logfile);
    sceKernelExitProcess(0);return 0;
}
