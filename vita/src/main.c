#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <vitaGL.h>
#include <stdio.h>
#include "touch.h"
#include "diagnostic_ui.h"
#include "arm_reloc.h"
#include "native_registry.h"
#include "android_log_adapter.h"
#include "jni_vm.h"
#include "bionic_memory.h"
int _newlib_heap_size_user=96*1024*1024;
/* Diagnostic renderer only. kubridge is a memory/cache bridge, NOT graphics. */
static FILE *logfile;
static int android_log_to_file(const SfLogRecord *record,void *context){
    FILE *file=(FILE *)context;char line[1280];
    int n=snprintf(line,sizeof line,"[android %d/%d] %s: %s%s\n",record->buffer,record->priority,record->tag,record->text,record->truncated?" [truncated]":"");
    if(n<0||(size_t)n>=sizeof line)return 0;
    return fwrite(line,1,(size_t)n,file)==(size_t)n&&fflush(file)==0;
}
static void probe(void){}
static int runtime_selftest(void){
    ArmScopeEntry entries[20];ArmSymbolScope scope;
    if(sf_bionic_memory_catalog(entries,20)!=20||arm_scope_init(&scope,entries,20))return 0;
    char *copy=sf_bionic_strdup("vita-abi-probe");
    int ok=copy&&sf_bionic_strlen(copy)==14;sf_bionic_free(copy);
    SfJavaVM *vm=sf_jni_vm_create("ux0:data/sf2vita/assets",4096);if(!vm)return 0;
    SfJniEnv *env=0;
    ok=ok&&(*vm)->GetEnv(vm,(void **)&env,0x10006)==0;
    if(env){
        SfJObject context=sf_jni_bridge_context(env);
        SfJObject cls=(*env)->GetObjectClass(env,context);
        SfJMethod method=(*env)->GetMethodID(env,cls,"getAssets","()Landroid/content/res/AssetManager;");
        SfJObject asset=(*env)->CallObjectMethodA(env,context,method,0);
        ok=ok&&asset&&sf_jni_bridge_assets(env,asset)!=0;
        (*env)->FindClass(env,"diagnostic/UnsupportedClass");
        SfJObject error=(*env)->ExceptionOccurred(env);
        ok=ok&&error&&(*env)->ExceptionCheck(env);
        (*env)->ExceptionClear(env);
        ok=ok&&(*env)->Throw(env,error)==0;
        (*env)->ExceptionClear(env);
    }
    ok=ok&&(*vm)->DetachCurrentThread(vm)==0;
    sf_jni_vm_release(vm);return ok;
}
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
    sf_log_configure(logfile?android_log_to_file:0,logfile,SF_LOG_INFO);
    int core_ok=selftest();
    int runtime_ok=runtime_selftest();core_ok=core_ok&&runtime_ok;
    ArmScopeEntry log_entries[5];ArmSymbolScope log_scope;
    core_ok=core_ok&&sf_log_target_catalog(log_entries,5)==5&&arm_scope_init(&log_scope,log_entries,5)==0;
    int logger_ok=__android_log_print(SF_LOG_INFO,"sf2vita","adapter check i=%d d=%.2f",7,1.25)==1;
    core_ok=core_ok&&logger_ok;
    if(logfile){fprintf(logfile,"SF2 diagnostic 0.4; Unity NOT started; core check=%s; JNI/memory probe=%s\n",core_ok?"PASS":"FAIL",runtime_ok?"PASS":"FAIL");fflush(logfile);}
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
