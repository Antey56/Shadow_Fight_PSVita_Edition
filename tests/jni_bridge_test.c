#include "jni_bridge.h"
#include <assert.h>
#include <stdio.h>
static SfJObject invoke_v(SfJniEnv *e,SfJObject o,SfJMethod m,...){
    va_list args;va_start(args,m);
    SfJObject result=(*e)->CallObjectMethodV(e,o,m,args);va_end(args);return result;
}
static SfJObject invoke_static_v(SfJniEnv *e,SfJObject cls,SfJMethod m,...){
    va_list args;va_start(args,m);
    SfJObject result=(*e)->CallStaticObjectMethodV(e,cls,m,args);va_end(args);return result;
}
int main(void){
    unsigned required[]={4,6,17,21,22,23,25,31,33,34,35,36,228};
    assert(!sf_jni_bridge_require(required,13));
    unsigned missing[]={4,94};assert(sf_jni_bridge_require(missing,2)<0);
    assert(sf_jni_bridge_require(0,0)<0);
    SfJniEnv *e=sf_jni_bridge_create(".",4096);assert(e);
    assert((*e)->GetVersion(e)==0x10006);
    SfJObject context=sf_jni_bridge_context(e);assert(context);
    SfJObject cls=(*e)->FindClass(e,"android/content/Context");assert(cls);
    SfJMethod m=(*e)->GetMethodID(e,cls,"getAssets","()Landroid/content/res/AssetManager;");assert(m);
    SfJObject a=(*e)->CallObjectMethod(e,context,m);assert(a);
    SfJObject av=invoke_v(e,context,m);assert(av);
    SfJObject aa=(*e)->CallObjectMethodA(e,context,m,0);assert(aa);
    assert(sf_jni_bridge_assets(e,a)==sf_jni_bridge_assets(e,av));
    assert(sf_jni_bridge_assets(e,a)==sf_jni_bridge_assets(e,aa));
    assert((*e)->GetObjectClass(e,a));
    SfJObject g=(*e)->NewGlobalRef(e,context);assert(g);
    (*e)->DeleteLocalRef(e,context);
    assert((*e)->CallObjectMethodA(e,g,m,0));
    (*e)->DeleteGlobalRef(e,g);
    assert(!(*e)->CallObjectMethodA(e,g,m,0));assert((*e)->ExceptionCheck(e));
    (*e)->ExceptionClear(e);assert(!(*e)->ExceptionCheck(e));
    assert(!(*e)->FindClass(e,"unknown/Class"));assert((*e)->ExceptionCheck(e));
    assert(!(*e)->CallObjectMethodA(e,aa,m,0));
    (*e)->ExceptionClear(e);
    assert(!(*e)->CallObjectMethodA(e,aa,m,0));assert((*e)->ExceptionCheck(e));
    (*e)->ExceptionClear(e);
    SfJObject meta=(*e)->FindClass(e,"java/lang/Class");assert(meta);
    SfJMethod lookup=(*e)->GetStaticMethodID(e,meta,"forName","(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");assert(lookup);
    SfJObject name=(*e)->NewStringUTF(e,"java.lang.String");assert(name);
    assert((*e)->CallStaticObjectMethod(e,meta,lookup,name,0,(SfJObject)0));
    assert(invoke_static_v(e,meta,lookup,name,0,(SfJObject)0));
    SfJObject longs=(*e)->NewLongArray(e,2);assert(longs);
    int64_t values[2]={INT64_C(-1099511627776),INT64_C(1099511627776)},output[2]={0};
    (*e)->SetLongArrayRegion(e,longs,0,2,values);
    (*e)->GetLongArrayRegion(e,longs,0,2,output);assert(output[0]==values[0]&&output[1]==values[1]);
    SfJObject buffer=(*e)->NewDirectByteBuffer(e,values,sizeof values);assert(buffer);
    assert((*e)->GetDirectBufferCapacity(e,buffer)==sizeof values);
    assert((*e)->GetDirectBufferAddress(e,buffer)==values);
    sf_jni_bridge_destroy(e);puts("restricted JNI table PASS");return 0;
}
