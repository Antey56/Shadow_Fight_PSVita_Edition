#ifndef SF_JNI_BRIDGE_H
#define SF_JNI_BRIDGE_H
#include <stdarg.h>
#include "jni_objects.h"
#include "jni_registration.h"
typedef void *SfJObject;
typedef void *SfJMethod;
typedef union {uint8_t z;int8_t b;uint16_t c;int16_t s;int32_t i;
               int64_t j;float f;double d;SfJObject l;} SfJValue;
struct SfJniTable;
typedef const struct SfJniTable *SfJniEnv;
struct SfJavaVMTable;
typedef const struct SfJavaVMTable *SfJavaVM;
typedef void (*SfJniUnused)(void);
#define SF_JNI_ARRAY_TYPES(X) X(Boolean,uint8_t,0) X(Byte,int8_t,1) X(Char,uint16_t,2) X(Short,int16_t,3) X(Int,int32_t,4) X(Long,int64_t,5) X(Float,float,6) X(Double,double,7)
/* JNI 1.6 slot layout; NULL entries are UNSUPPORTED, never callable.
   This restricted harness MUST NOT be passed to Unity/JNI_OnLoad yet. */
typedef struct SfJniTable {
    void *reserved[4]; /* 0..3 */
    int32_t (SF_ANDROID_ABI *GetVersion)(SfJniEnv *); /* 4 */
    SfJniUnused DefineClass; /* 5 */
    SfJObject (SF_ANDROID_ABI *FindClass)(SfJniEnv *,const char *); /* 6 */
    SfJniUnused unused7_12[6];
    int32_t (SF_ANDROID_ABI *Throw)(SfJniEnv *,SfJObject); /* 13 */
    SfJniUnused ThrowNew; /* 14: unsupported */
    SfJObject (SF_ANDROID_ABI *ExceptionOccurred)(SfJniEnv *); /* 15 */
    SfJniUnused ExceptionDescribe; /* 16: unsupported */
    void (SF_ANDROID_ABI *ExceptionClear)(SfJniEnv *); /* 17 */
    void (SF_ANDROID_ABI *FatalError)(SfJniEnv *,const char *); /* 18: does not return */
    SfJniUnused unused19_20[2];
    SfJObject (SF_ANDROID_ABI *NewGlobalRef)(SfJniEnv *,SfJObject); /* 21 */
    void (SF_ANDROID_ABI *DeleteGlobalRef)(SfJniEnv *,SfJObject); /* 22 */
    void (SF_ANDROID_ABI *DeleteLocalRef)(SfJniEnv *,SfJObject); /* 23 */
    SfJniUnused IsSameObject; /* 24 */
    SfJObject (SF_ANDROID_ABI *NewLocalRef)(SfJniEnv *,SfJObject); /* 25 */
    SfJniUnused unused26_30[5];
    SfJObject (SF_ANDROID_ABI *GetObjectClass)(SfJniEnv *,SfJObject); /* 31 */
    SfJniUnused IsInstanceOf; /* 32 */
    SfJMethod (SF_ANDROID_ABI *GetMethodID)(SfJniEnv *,SfJObject,const char *,const char *); /* 33 */
    SfJObject (SF_ANDROID_ABI *CallObjectMethod)(SfJniEnv *,SfJObject,SfJMethod,...); /* 34 */
    SfJObject (SF_ANDROID_ABI *CallObjectMethodV)(SfJniEnv *,SfJObject,SfJMethod,va_list); /* 35 */
    SfJObject (SF_ANDROID_ABI *CallObjectMethodA)(SfJniEnv *,SfJObject,SfJMethod,const SfJValue *); /* 36 */
    SfJniUnused unused37_112[76];
    SfJMethod (SF_ANDROID_ABI *GetStaticMethodID)(SfJniEnv *,SfJObject,const char *,const char *); /* 113 */
    SfJObject (SF_ANDROID_ABI *CallStaticObjectMethod)(SfJniEnv *,SfJObject,SfJMethod,...); /* 114 */
    SfJObject (SF_ANDROID_ABI *CallStaticObjectMethodV)(SfJniEnv *,SfJObject,SfJMethod,va_list); /* 115 */
    SfJObject (SF_ANDROID_ABI *CallStaticObjectMethodA)(SfJniEnv *,SfJObject,SfJMethod,const SfJValue *); /* 116 */
    SfJniUnused unused117_163[47];
    int32_t (SF_ANDROID_ABI *GetStringLength)(SfJniEnv *,SfJObject); /* 164 */
    SfJniUnused unused165_166[2];
    SfJObject (SF_ANDROID_ABI *NewStringUTF)(SfJniEnv *,const char *); /* 167 */
    int32_t (SF_ANDROID_ABI *GetStringUTFLength)(SfJniEnv *,SfJObject); /* 168 */
    const char *(SF_ANDROID_ABI *GetStringUTFChars)(SfJniEnv *,SfJObject,uint8_t *); /* 169 */
    void (SF_ANDROID_ABI *ReleaseStringUTFChars)(SfJniEnv *,SfJObject,const char *); /* 170 */
    int32_t (SF_ANDROID_ABI *GetArrayLength)(SfJniEnv *,SfJObject); /* 171 */
    SfJniUnused unused172_174[3];
#define SF_ARRAY_NEW(n,t,i) SfJObject (SF_ANDROID_ABI *New##n##Array)(SfJniEnv *,int32_t);
    SF_JNI_ARRAY_TYPES(SF_ARRAY_NEW) /* 175..182 */
#undef SF_ARRAY_NEW
#define SF_ARRAY_GET(n,t,i) t *(SF_ANDROID_ABI *Get##n##ArrayElements)(SfJniEnv *,SfJObject,uint8_t *);
    SF_JNI_ARRAY_TYPES(SF_ARRAY_GET) /* 183..190 */
#undef SF_ARRAY_GET
#define SF_ARRAY_RELEASE(n,t,i) void (SF_ANDROID_ABI *Release##n##ArrayElements)(SfJniEnv *,SfJObject,t *,int32_t);
    SF_JNI_ARRAY_TYPES(SF_ARRAY_RELEASE) /* 191..198 */
#undef SF_ARRAY_RELEASE
#define SF_ARRAY_GET_REGION(n,t,i) void (SF_ANDROID_ABI *Get##n##ArrayRegion)(SfJniEnv *,SfJObject,int32_t,int32_t,t *);
    SF_JNI_ARRAY_TYPES(SF_ARRAY_GET_REGION) /* 199..206 */
#undef SF_ARRAY_GET_REGION
#define SF_ARRAY_SET_REGION(n,t,i) void (SF_ANDROID_ABI *Set##n##ArrayRegion)(SfJniEnv *,SfJObject,int32_t,int32_t,const t *);
    SF_JNI_ARRAY_TYPES(SF_ARRAY_SET_REGION) /* 207..214 */
#undef SF_ARRAY_SET_REGION
    int32_t (SF_ANDROID_ABI *RegisterNatives)(SfJniEnv *,SfJObject,const SfJniNativeMethod *,int32_t); /* 215 */
    SfJniUnused unused216_218[3];
    int32_t (SF_ANDROID_ABI *GetJavaVM)(SfJniEnv *,SfJavaVM **); /* 219 */
    SfJniUnused unused220_227[8];
    uint8_t (SF_ANDROID_ABI *ExceptionCheck)(SfJniEnv *); /* 228 */
    SfJObject (SF_ANDROID_ABI *NewDirectByteBuffer)(SfJniEnv *,void *,int64_t); /* 229 */
    void *(SF_ANDROID_ABI *GetDirectBufferAddress)(SfJniEnv *,SfJObject); /* 230 */
    int64_t (SF_ANDROID_ABI *GetDirectBufferCapacity)(SfJniEnv *,SfJObject); /* 231 */
    SfJniUnused GetObjectRefType; /* 232 unsupported */
} SfJniTable;
/* Returned env address is owned, single-thread; no cross-env object transfer. */
SfJniEnv *sf_jni_bridge_create(const char *,size_t);
void sf_jni_bridge_destroy(SfJniEnv *);
SfJObject sf_jni_bridge_context(SfJniEnv *);
AAssetManager *sf_jni_bridge_assets(SfJniEnv *,SfJObject);
/* Capability allowlist gate: rejects absent/unimplemented slots, not a proof
   of Unity readiness. Caller must list all slots it intends to exercise. */
int sf_jni_bridge_require(const unsigned *,size_t);
/* Internal owner-thread VM integration; not for application use. */
void sf_jni_bridge_bind_vm(SfJniEnv *,SfJavaVM *,int (*)(void *),void *);
void sf_jni_bridge_clear_locals(SfJniEnv *);
/* Borrowed immutable policy must outlive env; ARM32 executable ranges must
   come from a separately validated loader. This API does not certify them. */
int sf_jni_bridge_configure_natives(SfJniEnv *,const NativeDeclaration *,size_t,
                                   const SfJniCodeRange *,size_t);
uintptr_t sf_jni_bridge_registered(SfJniEnv *,const char *,const char *,const char *);
#endif
