#include "jni_bridge.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef struct {SfJniEnv table;SfJniObjects *objects;SfJavaVM *vm;
                int (*guard)(void *);void *guard_context;} Bridge;
static const SfJniTable table;
static char get_assets_method;
#define SLOT_CHECK(field,n) typedef char check_##field[(offsetof(SfJniTable,field)==(n)*sizeof(SfJniUnused))?1:-1]
SLOT_CHECK(GetVersion,4);SLOT_CHECK(FindClass,6);SLOT_CHECK(ExceptionClear,17);
SLOT_CHECK(NewGlobalRef,21);SLOT_CHECK(NewLocalRef,25);SLOT_CHECK(GetObjectClass,31);
SLOT_CHECK(GetMethodID,33);SLOT_CHECK(CallObjectMethod,34);SLOT_CHECK(CallObjectMethodV,35);
SLOT_CHECK(CallObjectMethodA,36);SLOT_CHECK(ExceptionCheck,228);
SLOT_CHECK(GetJavaVM,219);
typedef char check_table_size[sizeof(SfJniTable)==233*sizeof(SfJniUnused)?1:-1];
static SfJniObjects *objects(SfJniEnv *e){
    if(!e||*e!=&table)return 0;
    Bridge *b=(Bridge *)e;
    return !b->guard||b->guard(b->guard_context)?b->objects:0;
}
static SfJniRef decode(SfJniObjects *o,SfJObject r){
    uintptr_t v=(uintptr_t)r;
    if(v>UINT32_MAX){sf_jni_error(o,SF_JNI_BAD_REF);return 0;}
    return (SfJniRef)v;
}
static SfJObject encode(SfJniRef r){return (SfJObject)(uintptr_t)r;}
static SF_ANDROID_ABI int32_t version(SfJniEnv *e){return objects(e)?0x10006:0;}
static SF_ANDROID_ABI SfJObject find_class(SfJniEnv *e,const char *name){
    SfJniObjects *o=objects(e);if(!o||sf_jni_exception(o))return 0;
    int kind=0;
    if(name&&!strcmp(name,"android/content/Context"))kind=3;
    if(name&&!strcmp(name,"android/content/res/AssetManager"))kind=4;
    if(!kind){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
    return encode(sf_jni_class(o,kind));
}
static SF_ANDROID_ABI void clear(SfJniEnv *e){sf_jni_exception_clear(objects(e));}
static SF_ANDROID_ABI uint8_t check(SfJniEnv *e){return sf_jni_exception(objects(e))!=0;}
static SF_ANDROID_ABI int32_t get_vm(SfJniEnv *e,SfJavaVM **out){
    if(!out)return -1;
    *out=0;
    if(!objects(e))return -1;
    *out=((Bridge *)e)->vm;return *out?0:-1;
}
static SF_ANDROID_ABI SfJObject global_ref(SfJniEnv *e,SfJObject r){
    SfJniObjects *o=objects(e);return encode(sf_jni_new_ref(o,decode(o,r),1));
}
static SF_ANDROID_ABI SfJObject local_ref(SfJniEnv *e,SfJObject r){
    SfJniObjects *o=objects(e);return encode(sf_jni_new_ref(o,decode(o,r),0));
}
static SF_ANDROID_ABI void delete_global(SfJniEnv *e,SfJObject r){
    SfJniObjects *o=objects(e);sf_jni_delete_ref(o,decode(o,r),1);
}
static SF_ANDROID_ABI void delete_local(SfJniEnv *e,SfJObject r){
    SfJniObjects *o=objects(e);sf_jni_delete_ref(o,decode(o,r),0);
}
static SF_ANDROID_ABI SfJObject object_class(SfJniEnv *e,SfJObject r){
    SfJniObjects *o=objects(e);int kind=sf_jni_kind(o,decode(o,r));
    if(kind!=1&&kind!=2){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
    return encode(sf_jni_class(o,kind+2));
}
static SF_ANDROID_ABI SfJMethod method(SfJniEnv *e,SfJObject cls,const char *name,const char *sig){
    SfJniObjects *o=objects(e);int kind=sf_jni_kind(o,decode(o,cls));
    if(!kind)return 0;
    if(kind==3&&name&&sig&&!strcmp(name,"getAssets")&&
       !strcmp(sig,"()Landroid/content/res/AssetManager;"))return &get_assets_method;
    sf_jni_error(o,SF_JNI_BAD_METHOD);return 0;
}
static SF_ANDROID_ABI SfJObject call_a(SfJniEnv *e,SfJObject receiver,SfJMethod id,const SfJValue *args){
    (void)args;SfJniObjects *o=objects(e);
    if(!o||sf_jni_exception(o))return 0;
    if(id!=&get_assets_method){sf_jni_error(o,SF_JNI_BAD_METHOD);return 0;}
    return encode(sf_jni_context_call(o,decode(o,receiver),"getAssets","()Landroid/content/res/AssetManager;"));
}
static SF_ANDROID_ABI SfJObject call_v(SfJniEnv *e,SfJObject receiver,SfJMethod id,va_list args){
    (void)args;return call_a(e,receiver,id,0);
}
static SF_ANDROID_ABI SfJObject call(SfJniEnv *e,SfJObject receiver,SfJMethod id,...){
    return call_a(e,receiver,id,0); /* Only the zero-argument method exists. */
}
static const SfJniTable table={
    .GetVersion=version,.FindClass=find_class,.ExceptionClear=clear,
    .NewGlobalRef=global_ref,.DeleteGlobalRef=delete_global,.DeleteLocalRef=delete_local,
    .NewLocalRef=local_ref,.GetObjectClass=object_class,.GetMethodID=method,
    .CallObjectMethod=call,.CallObjectMethodV=call_v,.CallObjectMethodA=call_a,
    .GetJavaVM=get_vm,.ExceptionCheck=check
};
SfJniEnv *sf_jni_bridge_create(const char *root,size_t limit){
    Bridge *b=calloc(1,sizeof(*b));if(!b)return 0;
    b->objects=sf_jni_objects_create(root,limit);
    if(!b->objects){free(b);return 0;}b->table=&table;return &b->table;
}
void sf_jni_bridge_destroy(SfJniEnv *e){
    if(objects(e)){Bridge *b=(Bridge *)e;sf_jni_objects_destroy(b->objects);free(b);}
}
SfJObject sf_jni_bridge_context(SfJniEnv *e){return encode(sf_jni_context(objects(e)));}
void sf_jni_bridge_bind_vm(SfJniEnv *e,SfJavaVM *vm,int (*guard)(void *),void *context){
    if(e&&*e==&table){Bridge *b=(Bridge *)e;b->vm=vm;b->guard=guard;b->guard_context=context;}
}
void sf_jni_bridge_clear_locals(SfJniEnv *e){sf_jni_clear_locals(objects(e));}
AAssetManager *sf_jni_bridge_assets(SfJniEnv *e,SfJObject r){
    SfJniObjects *o=objects(e);return sf_jni_asset_manager(o,decode(o,r));
}
int sf_jni_bridge_require(const unsigned *slots,size_t count){
    if(!slots||!count)return -1;
    for(size_t i=0;i<count;i++)switch(slots[i]){
        case 4:case 6:case 17:case 21:case 22:case 23:case 25:case 31:
        case 33:case 34:case 35:case 36:case 219:case 228:break;
        default:return -1;
    }
    return 0;
}
