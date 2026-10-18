#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>
#include <stdarg.h>
#include <vitaGL.h>
#include <stdio.h>
#include "touch.h"
#include "diagnostic_ui.h"
#include "arm_reloc.h"
#include "native_registry.h"
#include "android_log_adapter.h"
#include "jni_vm.h"
#include "bionic_memory.h"
int _newlib_heap_size_user=32*1024*1024;
/* Diagnostic renderer only. kubridge is a memory/cache bridge, NOT graphics. */
static int log_ready;
static void stage(const char *format,...){
    FILE *file=fopen("ux0:data/sf2vita-input.log","a");if(!file)return;
    fprintf(file,"[%llu us] ",(unsigned long long)sceKernelGetProcessTimeWide());
    va_list args;va_start(args,format);vfprintf(file,format,args);va_end(args);
    fputc('\n',file);fflush(file);fclose(file);
}
static void memory_snapshot(const char *where,int graphics_ready){
    SceKernelFreeMemorySizeInfo info={0};info.size=sizeof info;
    int rc=sceKernelGetFreeMemorySize(&info);
    stage("MEM %s kernel_rc=0x%08x user=%d cdram=%d phycont=%d bytes",where,(unsigned)rc,info.size_user,info.size_cdram,info.size_phycont);
    if(graphics_ready)for(int i=0;i<5;i++)stage("MEM %s vgl_pool=%d free=%lu total=%lu bytes",where,i,
        (unsigned long)vglMemFree((vglMemType)i),(unsigned long)vglMemTotal((vglMemType)i));
}
static int start_watcher(SceSize size,void *data){
    (void)size;(void)data;
    for(;;){SceCtrlData pad={0};
        if(sceCtrlPeekBufferPositive(0,&pad,1)>0&&(pad.buttons&SCE_CTRL_START))sceKernelExitProcess(0);
        sceKernelDelayThread(50000);
    }
    return 0;
}
static int android_log_to_file(const SfLogRecord *record,void *context){
    (void)context;
    stage("[android %d/%d] %s: %s%s",record->buffer,record->priority,record->tag,record->text,record->truncated?" [truncated]":"");
    return log_ready;
}
static void probe(void){}
static int runtime_selftest(void){
    ArmScopeEntry entries[20];ArmSymbolScope scope;
    int32_t values[4]={1,-2,3,4}; /* Borrowed buffer remains live through VM cleanup. */
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
        SfJObject meta=(*env)->FindClass(env,"java/lang/Class");
        SfJMethod for_name=(*env)->GetStaticMethodID(env,meta,"forName","(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
        SfJObject name=(*env)->NewStringUTF(env,"java.lang.String");
        SfJValue args[3]={{0},{0},{0}};args[0].l=name;
        SfJObject string_class=(*env)->CallStaticObjectMethodA(env,meta,for_name,args);
        const char *chars=(*env)->GetStringUTFChars(env,name,0);
        ok=ok&&string_class&&chars&&sf_bionic_strcmp(chars,"java.lang.String")==0;
        if(chars)(*env)->ReleaseStringUTFChars(env,name,chars);
        SfJObject numbers=(*env)->NewIntArray(env,4);
        int32_t readback[4]={0};
        (*env)->SetIntArrayRegion(env,numbers,0,4,values);
        int32_t *elements=(*env)->GetIntArrayElements(env,numbers,0);
        ok=ok&&numbers&&elements&&(*env)->GetArrayLength(env,numbers)==4;
        if(elements){
            elements[1]=-9;(*env)->ReleaseIntArrayElements(env,numbers,elements,1);
            (*env)->GetIntArrayRegion(env,numbers,0,4,readback);
            ok=ok&&readback[0]==1&&readback[1]==-9&&readback[3]==4;
            (*env)->ReleaseIntArrayElements(env,numbers,elements,2);
        }
        SfJObject buffer=(*env)->NewDirectByteBuffer(env,values,sizeof values);
        ok=ok&&buffer&&(*env)->GetDirectBufferAddress(env,buffer)==values&&
            (*env)->GetDirectBufferCapacity(env,buffer)==sizeof values;
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
    stage("touch action=%d index=%d count=%d time=%llu",e->action,e->index,e->count,(unsigned long long)e->time_us);
    for(int i=0;i<e->count;i++)stage("  id=%d x=%.1f y=%.1f",e->points[i].id,e->points[i].x,e->points[i].y);
}
int main(void){
    FILE *reset=fopen("ux0:data/sf2vita-input.log","w");log_ready=reset!=0;if(reset)fclose(reset);
    stage("SF2 diagnostic 0.7-gfxtrace1; Unity NOT started");
    memory_snapshot("entry",0);
    sf_log_configure(log_ready?android_log_to_file:0,0,SF_LOG_INFO);
    stage("BEFORE_CORE_PROBES");
    int core_ok=selftest();int runtime_ok=runtime_selftest();core_ok=core_ok&&runtime_ok;
    ArmScopeEntry log_entries[5];ArmSymbolScope log_scope;
    core_ok=core_ok&&sf_log_target_catalog(log_entries,5)==5&&arm_scope_init(&log_scope,log_entries,5)==0;
    int logger_ok=__android_log_print(SF_LOG_INFO,"sf2vita","adapter check i=%d d=%.2f",7,1.25)==1;
    core_ok=core_ok&&logger_ok;
    stage("AFTER_CORE_PROBES core=%s JNI_memory_reflection_arrays=%s",core_ok?"PASS":"FAIL",runtime_ok?"PASS":"FAIL");
    int sampling=sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);stage("INPUT sampling_rc=%d",sampling);
    SceUID watcher=sceKernelCreateThread("sf2_start_exit",start_watcher,sceKernelGetThreadCurrentPriority(),8192,0,0,0);
    int watcher_rc=watcher<0?watcher:sceKernelStartThread(watcher,0,0);
    stage("START_WATCHER thread=%d start_rc=0x%08x; independent of graphics",watcher,(unsigned)watcher_rc);
    memory_snapshot("before_vglInit",0);
    stage("BEFORE_VGL_INIT legacy=0 circular=1048576 buffers=2 resolution=960x544 MSAA=NONE");
    vglSetCircularPoolSize(1024*1024);vglSetDisplayBufferCount(2);
    /* In pinned vitaGL, return value means resolution fallback, NOT success.
       False is the normal 960x544 path; never exit based on this boolean. */
    GLboolean fallback=vglInitExtended(0,960,544,16*1024*1024,SCE_GXM_MULTISAMPLE_NONE);
    stage("AFTER_VGL_INIT resolution_fallback=%d",(int)fallback);
    memory_snapshot("after_vglInit",1);
    glViewport(0,0,960,544);
    TouchState state={0};SceCtrlData pad={0};pad.lx=pad.ly=128;
    unsigned long frame=0;stage("ENTER_MAIN_LOOP");
    for(;;){
        SceCtrlData current={0};
        if(frame<3)stage("BEFORE_INPUT frame=%lu",frame);
        int rc=sceCtrlPeekBufferPositive(0,&current,1);
        if(rc>0)pad=current;else{pad.buttons=0;pad.lx=pad.ly=128;}
        if(frame<3)stage("AFTER_INPUT frame=%lu rc=%d buttons=0x%08x lx=%u ly=%u",frame,rc,(unsigned)pad.buttons,pad.lx,pad.ly);
        if(pad.buttons&SCE_CTRL_START){stage("EXIT_REQUEST START main");break;}
        unsigned b=0;
#define MAP(v,a) if(pad.buttons&(v))b|=(a)
        MAP(SCE_CTRL_LEFT,B_LEFT);MAP(SCE_CTRL_RIGHT,B_RIGHT);MAP(SCE_CTRL_UP,B_UP);MAP(SCE_CTRL_DOWN,B_DOWN);
        MAP(SCE_CTRL_CROSS,B_CROSS);MAP(SCE_CTRL_SQUARE,B_SQUARE);MAP(SCE_CTRL_TRIANGLE,B_TRIANGLE);MAP(SCE_CTRL_CIRCLE,B_CIRCLE);
        if(rc>0)touch_step(&state,b,(pad.lx-128)/128.0f,(pad.ly-128)/128.0f,pad.timeStamp,sink,0);
        else touch_cancel(&state,(uint64_t)sceKernelGetSystemTimeWide(),sink,0);
        if(frame<3)stage("BEFORE_DRAW frame=%lu",frame);
        diagnostic_ui(&state,log_ready,core_ok);
        if(frame<3)stage("AFTER_DRAW frame=%lu gl_error=0x%04x",frame,(unsigned)glGetError());
        if(frame<3)stage("BEFORE_SWAP frame=%lu",frame);
        vglSwapBuffers(GL_FALSE);
        if(frame<3)stage("AFTER_SWAP frame=%lu",frame);
        if(frame==0)memory_snapshot("after_first_swap",1);
        frame++;
        if(frame%600==0)stage("HEARTBEAT frame=%lu vgl_frame=%lu",frame,(unsigned long)vglGetFrameNumber());
    }
    touch_cancel(&state,(uint64_t)sceKernelGetSystemTimeWide(),sink,0);stage("BEFORE_EXIT_PROCESS frames=%lu",frame);
    sceKernelExitProcess(0);return 0;
}
