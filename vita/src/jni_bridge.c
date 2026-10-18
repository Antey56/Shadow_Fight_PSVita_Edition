#include "jni_bridge.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef struct {SfJniEnv table;SfJniObjects *objects;SfJavaVM *vm;
                int (*guard)(void *);void *guard_context;
                const NativeDeclaration *declarations;size_t declaration_count;
                const SfJniCodeRange *ranges;size_t range_count;SfNativeCatalog *natives;
                struct {uint64_t identity;char *bytes;} leases[16];} Bridge;
static const char *extra_classes[]={
    "com/unity3d/player/UnityPlayer","com/google/androidgamesdk/ChoreographerCallback",
    "com/google/androidgamesdk/SwappyDisplayManager","com/unity3d/player/ReflectionHelper",
    "com/unity3d/player/Camera2Wrapper","com/unity3d/player/HFPStatus",
    "com/unity3d/player/AudioVolumeHandler","com/unity3d/player/OrientationLockListener",
    "com/unity3d/player/UnityCoreAssetPacksStatusCallbacks","com/unity3d/player/GoogleARCoreApi"};
static const char *class_name(int kind){
    if(kind==3)return "android/content/Context";
    if(kind==4)return "android/content/res/AssetManager";
    return kind>=6&&kind<=15?extra_classes[kind-6]:0;
}
static const SfJniTable table;
static char get_assets_method;
static char get_class_loader_method;
static char for_name_method;
static int is_class(int kind){return kind==3||kind==4||(kind>=6&&kind<=16)||kind==18||kind==20||(kind>=41&&kind<=50);}
#define SLOT_CHECK(field,n) typedef char check_##field[(offsetof(SfJniTable,field)==(n)*sizeof(SfJniUnused))?1:-1]
SLOT_CHECK(GetVersion,4);SLOT_CHECK(FindClass,6);SLOT_CHECK(ExceptionClear,17);
SLOT_CHECK(FatalError,18);
SLOT_CHECK(NewGlobalRef,21);SLOT_CHECK(NewLocalRef,25);SLOT_CHECK(GetObjectClass,31);
SLOT_CHECK(GetMethodID,33);SLOT_CHECK(CallObjectMethod,34);SLOT_CHECK(CallObjectMethodV,35);
SLOT_CHECK(CallObjectMethodA,36);SLOT_CHECK(ExceptionCheck,228);
SLOT_CHECK(GetJavaVM,219);
SLOT_CHECK(Throw,13);SLOT_CHECK(ExceptionOccurred,15);
SLOT_CHECK(RegisterNatives,215);
SLOT_CHECK(GetStaticMethodID,113);SLOT_CHECK(CallStaticObjectMethodA,116);
SLOT_CHECK(GetStringLength,164);SLOT_CHECK(NewStringUTF,167);SLOT_CHECK(ReleaseStringUTFChars,170);
SLOT_CHECK(GetArrayLength,171);SLOT_CHECK(NewBooleanArray,175);SLOT_CHECK(NewDoubleArray,182);
SLOT_CHECK(GetBooleanArrayElements,183);SLOT_CHECK(ReleaseBooleanArrayElements,191);
SLOT_CHECK(GetBooleanArrayRegion,199);SLOT_CHECK(SetBooleanArrayRegion,207);
SLOT_CHECK(NewDirectByteBuffer,229);SLOT_CHECK(GetDirectBufferCapacity,231);
typedef char check_table_size[sizeof(SfJniTable)==233*sizeof(SfJniUnused)?1:-1];
typedef char check_jvalue_size[sizeof(SfJValue)==8?1:-1];
typedef char check_native_method_size[sizeof(SfJniNativeMethod)==3*sizeof(void *)?1:-1];
typedef char check_native_function_offset[offsetof(SfJniNativeMethod,fnPtr)==2*sizeof(void *)?1:-1];
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
    if(name&&!strcmp(name,"java/lang/Class"))kind=16;
    if(name&&!strcmp(name,"java/lang/ClassLoader"))kind=18;
    if(name&&!strcmp(name,"java/lang/String"))kind=20;
    if(name&&!strcmp(name,"java/nio/ByteBuffer"))kind=41;
    if(name&&!strcmp(name,"java/nio/DirectByteBuffer"))kind=42;
    static const char *array_names[]={"[Z","[B","[C","[S","[I","[J","[F","[D"};
    if(name)for(unsigned i=0;i<8;i++)if(!strcmp(name,array_names[i]))kind=(int)i+43;
    Bridge *b=(Bridge *)e;
    if(name&&b->natives)for(unsigned i=0;i<10;i++)if(!strcmp(name,extra_classes[i]))
        for(size_t j=0;j<b->declaration_count;j++)if(!strcmp(name,b->declarations[j].class_name))kind=(int)i+6;
    if(!kind){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
    return encode(sf_jni_class(o,kind));
}
static SF_ANDROID_ABI void clear(SfJniEnv *e){sf_jni_exception_clear(objects(e));}
static SF_ANDROID_ABI uint8_t check(SfJniEnv *e){return sf_jni_exception(objects(e))!=0;}
static SF_ANDROID_ABI SfJObject occurred(SfJniEnv *e){return encode(sf_jni_exception_ref(objects(e)));}
static SF_ANDROID_ABI int32_t throw_object(SfJniEnv *e,SfJObject r){
    SfJniObjects *o=objects(e);return sf_jni_throw(o,decode(o,r));
}
static SF_ANDROID_ABI int32_t get_vm(SfJniEnv *e,SfJavaVM **out){
    if(!out)return -1;
    *out=0;
    if(!objects(e))return -1;
    *out=((Bridge *)e)->vm;return *out?0:-1;
}
static SF_ANDROID_ABI int32_t register_natives(SfJniEnv *e,SfJObject cls,const SfJniNativeMethod *methods,int32_t count){
    SfJniObjects *o=objects(e);if(!o||sf_jni_exception(o))return -1;
    Bridge *b=(Bridge *)e;int kind=sf_jni_kind(o,decode(o,cls));
    const char *name=class_name(kind);
    if(!name){sf_jni_error(o,SF_JNI_BAD_TYPE);return -1;}
    if(!b->natives){sf_jni_error(o,SF_JNI_BAD_METHOD);return -1;}
    int rc=sf_jni_register_batch(b->natives,b->declarations,b->declaration_count,
                               b->ranges,b->range_count,name,methods,count);
    if(rc)sf_jni_error(o,rc==-3?SF_JNI_CAPACITY:rc==-4||rc==-2?SF_JNI_BAD_METHOD:SF_JNI_BAD_REF);
    return rc? -1:0;
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
    if(is_class(kind))return encode(sf_jni_class(o,16));
    if(kind==17)return encode(sf_jni_class(o,18));
    if(kind==19)return encode(sf_jni_class(o,20));
    if(kind==40)return encode(sf_jni_class(o,42));
    if(kind>=32&&kind<=39)return encode(sf_jni_class(o,kind+11));
    if(kind!=1&&kind!=2){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
    return encode(sf_jni_class(o,kind+2));
}
static SF_ANDROID_ABI SfJMethod method(SfJniEnv *e,SfJObject cls,const char *name,const char *sig){
    SfJniObjects *o=objects(e);int kind=sf_jni_kind(o,decode(o,cls));
    if(!kind)return 0;
    if(kind==16&&name&&sig&&!strcmp(name,"getClassLoader")&&
       !strcmp(sig,"()Ljava/lang/ClassLoader;"))return &get_class_loader_method;
    if(kind==3&&name&&sig&&!strcmp(name,"getAssets")&&
       !strcmp(sig,"()Landroid/content/res/AssetManager;"))return &get_assets_method;
    sf_jni_error(o,SF_JNI_BAD_METHOD);return 0;
}
static SF_ANDROID_ABI SfJObject call_a(SfJniEnv *e,SfJObject receiver,SfJMethod id,const SfJValue *args){
    (void)args;SfJniObjects *o=objects(e);
    if(!o||sf_jni_exception(o))return 0;
    if(id==&get_class_loader_method){
        if(!is_class(sf_jni_kind(o,decode(o,receiver)))){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
        return encode(sf_jni_class_loader(o));
    }
    if(id!=&get_assets_method){sf_jni_error(o,SF_JNI_BAD_METHOD);return 0;}
    return encode(sf_jni_context_call(o,decode(o,receiver),"getAssets","()Landroid/content/res/AssetManager;"));
}
static SF_ANDROID_ABI SfJObject call_v(SfJniEnv *e,SfJObject receiver,SfJMethod id,va_list args){
    (void)args;return call_a(e,receiver,id,0);
}
static SF_ANDROID_ABI SfJObject call(SfJniEnv *e,SfJObject receiver,SfJMethod id,...){
    return call_a(e,receiver,id,0); /* Only the zero-argument method exists. */
}
static SF_ANDROID_ABI SfJObject new_string(SfJniEnv *e,const char *s){return encode(sf_jni_string_utf(objects(e),s));}
static int32_t string_length(SfJniEnv *e,SfJObject r,int utf){
    SfJniObjects *o=objects(e);size_t bytes=0,units=0;
    if(!o||sf_jni_exception(o)||sf_jni_string_view(o,decode(o,r),0,&bytes,&units,0))return 0;
    return (int32_t)(utf?bytes:units);
}
static SF_ANDROID_ABI int32_t utf_length(SfJniEnv *e,SfJObject r){return string_length(e,r,1);}
static SF_ANDROID_ABI int32_t utf16_length(SfJniEnv *e,SfJObject r){return string_length(e,r,0);}
static SF_ANDROID_ABI const char *utf_chars(SfJniEnv *e,SfJObject r,uint8_t *is_copy){
    if(is_copy)*is_copy=0;
    SfJniObjects *o=objects(e);const char *text;size_t bytes;uint64_t identity;
    if(!o||sf_jni_exception(o)||sf_jni_string_view(o,decode(o,r),&text,&bytes,0,&identity))return 0;
    Bridge *b=(Bridge *)e;
    for(size_t i=0;i<16;i++)if(!b->leases[i].bytes){
        char *copy=malloc(bytes+1);if(!copy){sf_jni_error(o,SF_JNI_CAPACITY);return 0;}
        memcpy(copy,text,bytes+1);b->leases[i].bytes=copy;b->leases[i].identity=identity;
        if(is_copy)*is_copy=1;
        return copy;
    }
    sf_jni_error(o,SF_JNI_CAPACITY);return 0;
}
static SF_ANDROID_ABI void release_utf(SfJniEnv *e,SfJObject r,const char *chars){
    SfJniObjects *o=objects(e);uint64_t identity;
    if(!chars){sf_jni_error(o,SF_JNI_BAD_REF);return;}
    if(!o||sf_jni_string_view(o,decode(o,r),0,0,0,&identity))return;
    Bridge *b=(Bridge *)e;
    for(size_t i=0;i<16;i++)if(b->leases[i].bytes==chars&&b->leases[i].identity==identity){
        free(b->leases[i].bytes);b->leases[i].bytes=0;return;
    }
    sf_jni_error(o,SF_JNI_BAD_REF);
}
static SF_ANDROID_ABI SfJMethod static_method(SfJniEnv *e,SfJObject cls,const char *name,const char *sig){
    SfJniObjects *o=objects(e);int kind=sf_jni_kind(o,decode(o,cls));
    if(kind==16&&name&&sig&&!strcmp(name,"forName")&&
       !strcmp(sig,"(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;"))return &for_name_method;
    sf_jni_error(o,SF_JNI_BAD_METHOD);return 0;
}
static SF_ANDROID_ABI SfJObject static_call_a(SfJniEnv *e,SfJObject cls,SfJMethod id,const SfJValue *args){
    SfJniObjects *o=objects(e);if(!o||sf_jni_exception(o))return 0;
    if(id!=&for_name_method||sf_jni_kind(o,decode(o,cls))!=16||!args){sf_jni_error(o,SF_JNI_BAD_METHOD);return 0;}
    /* Only synthetic declared classes, no Java class initialization. */
    if(args[1].z){sf_jni_error(o,SF_JNI_BAD_METHOD);return 0;}
    if(args[2].l&&sf_jni_kind(o,decode(o,args[2].l))!=17){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
    const char *text;size_t bytes;
    if(sf_jni_string_view(o,decode(o,args[0].l),&text,&bytes,0,0))return 0;
    char name[128];if(bytes>=sizeof name){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
    for(size_t i=0;i<bytes;i++){
        unsigned c=(unsigned char)text[i];
        if(c<33||c>126||c=='/'||c=='['){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
        name[i]=c=='.'?'/':(char)c;
    }
    name[bytes]=0;
    if(!args[2].l&&!strncmp(name,"com/",4)){sf_jni_error(o,SF_JNI_BAD_TYPE);return 0;}
    return find_class(e,name);
}
static SF_ANDROID_ABI SfJObject static_call_v(SfJniEnv *e,SfJObject cls,SfJMethod id,va_list values){
    SfJniObjects *o=objects(e);
    if(!o||sf_jni_exception(o)||id!=&for_name_method){sf_jni_error(o,SF_JNI_BAD_METHOD);return 0;}
    SfJValue args[3];args[0].l=va_arg(values,SfJObject);args[1].z=va_arg(values,int)!=0;args[2].l=va_arg(values,SfJObject);
    return static_call_a(e,cls,id,args);
}
static SF_ANDROID_ABI SfJObject static_call(SfJniEnv *e,SfJObject cls,SfJMethod id,...){
    va_list values;va_start(values,id);SfJObject result=static_call_v(e,cls,id,values);va_end(values);return result;
}
static void release_leases(Bridge *b){
    for(size_t i=0;i<16;i++){free(b->leases[i].bytes);b->leases[i].bytes=0;}
}
#define ARRAY_CALLBACKS(n,t,i) \
static SF_ANDROID_ABI SfJObject new_##n(SfJniEnv *e,int32_t length){return encode(sf_jni_array_new(objects(e),i,length));} \
static SF_ANDROID_ABI t *elements_##n(SfJniEnv *e,SfJObject a,uint8_t *copy){SfJniObjects *o=objects(e);return sf_jni_array_elements(o,decode(o,a),i,copy);} \
static SF_ANDROID_ABI void release_##n(SfJniEnv *e,SfJObject a,t *copy,int32_t mode){SfJniObjects *o=objects(e);sf_jni_array_release(o,decode(o,a),i,copy,mode);} \
static SF_ANDROID_ABI void get_region_##n(SfJniEnv *e,SfJObject a,int32_t start,int32_t count,t *buffer){SfJniObjects *o=objects(e);sf_jni_array_region(o,decode(o,a),i,start,count,buffer,0);} \
static SF_ANDROID_ABI void set_region_##n(SfJniEnv *e,SfJObject a,int32_t start,int32_t count,const t *buffer){SfJniObjects *o=objects(e);sf_jni_array_region(o,decode(o,a),i,start,count,(void *)buffer,1);}
SF_JNI_ARRAY_TYPES(ARRAY_CALLBACKS)
#undef ARRAY_CALLBACKS
static SF_ANDROID_ABI int32_t array_length(SfJniEnv *e,SfJObject a){
    SfJniObjects *o=objects(e);int32_t length=0;
    if(!o||sf_jni_exception(o)||sf_jni_array_view(o,decode(o,a),-1,0,0,&length,0))return 0;
    return length;
}
static SF_ANDROID_ABI SfJObject new_direct(SfJniEnv *e,void *address,int64_t capacity){return encode(sf_jni_direct_buffer(objects(e),address,capacity));}
static SF_ANDROID_ABI void *direct_address(SfJniEnv *e,SfJObject a){
    SfJniObjects *o=objects(e);void *address=0;
    if(!o||sf_jni_exception(o)||sf_jni_direct_view(o,decode(o,a),&address,0))return 0;
    return address;
}
static SF_ANDROID_ABI int64_t direct_capacity(SfJniEnv *e,SfJObject a){
    SfJniObjects *o=objects(e);int64_t capacity=-1;
    if(!o||sf_jni_exception(o)||sf_jni_direct_view(o,decode(o,a),0,&capacity))return -1;
    return capacity;
}
/* FatalError is terminal even with a pending exception or invalid env.
   Bound diagnostic output; never interpret the supplied message as a format. */
static SF_ANDROID_ABI void fatal_error(SfJniEnv *e,const char *message){
    (void)e;
    const char *text=message?message:"(null)";
#ifdef __vita__
    FILE *file=fopen("ux0:data/sf2vita-input.log","a");
    if(file){fprintf(file,"JNI_FATAL: %.1024s\n",text);fflush(file);fclose(file);}
#endif
    fprintf(stderr,"JNI_FATAL: %.1024s\n",text);fflush(stderr);
    abort();
}
static const SfJniTable table={
    .GetVersion=version,.FindClass=find_class,.ExceptionClear=clear,
    .Throw=throw_object,.ExceptionOccurred=occurred,.FatalError=fatal_error,
    .NewGlobalRef=global_ref,.DeleteGlobalRef=delete_global,.DeleteLocalRef=delete_local,
    .NewLocalRef=local_ref,.GetObjectClass=object_class,.GetMethodID=method,
    .CallObjectMethod=call,.CallObjectMethodV=call_v,.CallObjectMethodA=call_a,
    .GetJavaVM=get_vm,.ExceptionCheck=check,.RegisterNatives=register_natives,
    .GetStaticMethodID=static_method,.CallStaticObjectMethod=static_call,
    .CallStaticObjectMethodV=static_call_v,.CallStaticObjectMethodA=static_call_a,
    .GetStringLength=utf16_length,.NewStringUTF=new_string,.GetStringUTFLength=utf_length,
    .GetStringUTFChars=utf_chars,.ReleaseStringUTFChars=release_utf,
    .GetArrayLength=array_length,
#define ARRAY_INITIALIZERS(n,t,i) .New##n##Array=new_##n,.Get##n##ArrayElements=elements_##n,.Release##n##ArrayElements=release_##n,.Get##n##ArrayRegion=get_region_##n,.Set##n##ArrayRegion=set_region_##n,
    SF_JNI_ARRAY_TYPES(ARRAY_INITIALIZERS)
#undef ARRAY_INITIALIZERS
    .NewDirectByteBuffer=new_direct,.GetDirectBufferAddress=direct_address,.GetDirectBufferCapacity=direct_capacity
};
SfJniEnv *sf_jni_bridge_create(const char *root,size_t limit){
    Bridge *b=calloc(1,sizeof(*b));if(!b)return 0;
    b->objects=sf_jni_objects_create(root,limit);
    if(!b->objects){free(b);return 0;}b->table=&table;return &b->table;
}
void sf_jni_bridge_destroy(SfJniEnv *e){
    if(objects(e)){Bridge *b=(Bridge *)e;release_leases(b);free(b->natives);sf_jni_objects_destroy(b->objects);free(b);}
}
int sf_jni_bridge_configure_natives(SfJniEnv *e,const NativeDeclaration *decl,size_t nd,
                                   const SfJniCodeRange *ranges,size_t nr){
    if(!objects(e)||!decl||!nd||nd>128||!ranges||!nr||nr>16)return -1;
    Bridge *b=(Bridge *)e;if(b->natives)return -1;
    for(size_t i=0;i<nd;i++)if(!decl[i].class_name||!decl[i].name||!decl[i].descriptor)return -1;
    for(size_t i=0;i<nr;i++)if(ranges[i].begin>=ranges[i].end)return -1;
    b->natives=calloc(1,sizeof(*b->natives));if(!b->natives)return -1;
    b->declarations=decl;b->declaration_count=nd;b->ranges=ranges;b->range_count=nr;return 0;
}
uintptr_t sf_jni_bridge_registered(SfJniEnv *e,const char *cls,const char *name,const char *sig){
    return objects(e)?sf_jni_registered(((Bridge *)e)->natives,cls,name,sig):0;
}
SfJObject sf_jni_bridge_context(SfJniEnv *e){return encode(sf_jni_context(objects(e)));}
void sf_jni_bridge_bind_vm(SfJniEnv *e,SfJavaVM *vm,int (*guard)(void *),void *context){
    if(e&&*e==&table){Bridge *b=(Bridge *)e;b->vm=vm;b->guard=guard;b->guard_context=context;}
}
void sf_jni_bridge_clear_locals(SfJniEnv *e){
    SfJniObjects *o=objects(e);if(o){release_leases((Bridge *)e);sf_jni_clear_locals(o);}
}
AAssetManager *sf_jni_bridge_assets(SfJniEnv *e,SfJObject r){
    SfJniObjects *o=objects(e);return sf_jni_asset_manager(o,decode(o,r));
}
int sf_jni_bridge_require(const unsigned *slots,size_t count){
    if(!slots||!count)return -1;
    for(size_t i=0;i<count;i++)switch(slots[i]){
        case 4:case 6:case 13:case 15:case 17:case 18:case 21:case 22:case 23:case 25:case 31:
        case 33:case 34:case 35:case 36:case 215:case 219:case 228:break;
        case 113:case 114:case 115:case 116:case 164:case 167:case 168:case 169:case 170:break;
        case 171:case 229:case 230:case 231:break;
        default:if(slots[i]<175||slots[i]>214)return -1;break;
    }
    return 0;
}
