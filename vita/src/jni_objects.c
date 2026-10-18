#include "jni_objects.h"
#include <stdlib.h>
#include <string.h>
#define REF_COUNT 64
#define GEN_MAX 0xffffffu
typedef struct {size_t refs,bytes,units;uint64_t identity;char data[];} JString;
typedef struct {uint32_t generation;unsigned char kind,global;int error;
                uint64_t exception_id;JString *string;} RefSlot;
struct SfJniObjects {AAssetManager *manager;RefSlot slots[REF_COUNT];int exception;
                     uint64_t exception_id,serial;};
static void fail(SfJniObjects *e,int error){
    if(e&&!e->exception){
        e->exception=error;
        /* Saturation never wraps identity; further IDs are unavailable. */
        e->exception_id=e->serial<UINT64_MAX?++e->serial:0;
    }
}
static RefSlot *resolve(SfJniObjects *e,SfJniRef r){
    unsigned index=r&255u;uint32_t gen=r>>8;
    if(!e||!index||index>REF_COUNT||!gen){fail(e,SF_JNI_BAD_REF);return 0;}
    RefSlot *s=&e->slots[index-1];
    if(!s->kind||s->generation!=gen){fail(e,SF_JNI_BAD_REF);return 0;}
    return s;
}
static SfJniRef allocate_raw(SfJniObjects *e,unsigned kind,int global){
    if(!e)return 0;
    for(unsigned i=0;i<REF_COUNT;i++){
        RefSlot *s=&e->slots[i];
        if(!s->kind&&s->generation<GEN_MAX){
            s->generation++;s->kind=(unsigned char)kind;s->global=(unsigned char)!!global;
            s->error=0;s->exception_id=0;s->string=0;
            return (s->generation<<8)|(i+1);
        }
    }
    fail(e,SF_JNI_CAPACITY);return 0;
}
static SfJniRef allocate(SfJniObjects *e,unsigned kind,int global){
    return !e||e->exception?0:allocate_raw(e,kind,global);
}
static void release_slot(RefSlot *s){
    if(s->kind==19&&s->string&&!--s->string->refs)free(s->string);
    s->string=0;s->kind=0;
}
SfJniRef sf_jni_string_utf(SfJniObjects *e,const char *text){
    if(!e||e->exception)return 0;
    if(!text){fail(e,SF_JNI_BAD_REF);return 0;}
    size_t bytes=0,units=0;
    while(bytes<=64u*1024u&&text[bytes])bytes++;
    if(bytes>64u*1024u){fail(e,SF_JNI_CAPACITY);return 0;}
    for(size_t i=0;i<bytes;units++){
        unsigned a=(unsigned char)text[i];
        if(a<128){i++;continue;}
        if(i+1<bytes){
            unsigned b=(unsigned char)text[i+1];
            if((a==0xc0&&b==0x80)||(a>=0xc2&&a<=0xdf&&b>=0x80&&b<=0xbf)){i+=2;continue;}
            if(a>=0xe0&&a<=0xef&&i+2<bytes){
                unsigned c=(unsigned char)text[i+2];
                if(b>=0x80&&b<=0xbf&&c>=0x80&&c<=0xbf&&(a!=0xe0||b>=0xa0)){i+=3;continue;}
            }
        }
        fail(e,SF_JNI_BAD_TYPE);return 0;
    }
    if(e->serial==UINT64_MAX){fail(e,SF_JNI_CAPACITY);return 0;}
    JString *s=malloc(sizeof(*s)+bytes+1);if(!s){fail(e,SF_JNI_CAPACITY);return 0;}
    SfJniRef r=allocate(e,19,0);if(!r){free(s);return 0;}
    s->refs=1;s->bytes=bytes;s->units=units;s->identity=++e->serial;memcpy(s->data,text,bytes+1);
    e->slots[(r&255u)-1].string=s;return r;
}
int sf_jni_string_view(SfJniObjects *e,SfJniRef r,const char **text,size_t *bytes,size_t *units,uint64_t *identity){
    RefSlot *s=resolve(e,r);if(!s)return -1;
    if(s->kind!=19){fail(e,SF_JNI_BAD_TYPE);return -1;}
    if(text)*text=s->string->data;
    if(bytes)*bytes=s->string->bytes;
    if(units)*units=s->string->units;
    if(identity)*identity=s->string->identity;
    return 0;
}
SfJniRef sf_jni_exception_ref(SfJniObjects *e){
    if(!e||!e->exception||!e->exception_id)return 0;
    SfJniRef r=allocate_raw(e,5,0);
    if(r){RefSlot *s=&e->slots[(r&255u)-1];s->error=e->exception;s->exception_id=e->exception_id;}
    return r;
}
int sf_jni_throwable_code(SfJniObjects *e,SfJniRef r){
    RefSlot *s=resolve(e,r);return s&&s->kind==5?s->error:0;
}
int sf_jni_throw(SfJniObjects *e,SfJniRef r){
    if(!e||e->exception)return -1; /* First pending exception is preserved. */
    RefSlot *s=resolve(e,r);if(!s)return -1;
    if(s->kind!=5){fail(e,SF_JNI_BAD_TYPE);return -1;}
    e->exception=s->error;e->exception_id=s->exception_id;return 0;
}
int sf_jni_kind(SfJniObjects *e,SfJniRef r){
    if(!e||e->exception)return 0;
    RefSlot *s=resolve(e,r);return s?s->kind:0;
}
SfJniRef sf_jni_class(SfJniObjects *e,int kind){
    if(kind!=3&&kind!=4&&kind!=18&&kind!=20&&(kind<6||kind>16)){fail(e,SF_JNI_BAD_TYPE);return 0;}
    return allocate(e,(unsigned)kind,0);
}
SfJniRef sf_jni_class_loader(SfJniObjects *e){return allocate(e,17,0);}
void sf_jni_error(SfJniObjects *e,int error){fail(e,error);}
SfJniObjects *sf_jni_objects_create(const char *root,size_t limit){
    SfJniObjects *e=calloc(1,sizeof(*e));if(!e)return 0;
    e->manager=sf_asset_manager_create(root,limit);
    if(!e->manager){free(e);return 0;}return e;
}
void sf_jni_objects_destroy(SfJniObjects *e){if(e){
    for(size_t i=0;i<REF_COUNT;i++)release_slot(&e->slots[i]);
    sf_asset_manager_destroy(e->manager);free(e);
}}
SfJniRef sf_jni_context(SfJniObjects *e){return allocate(e,1,0);}
SfJniRef sf_jni_new_ref(SfJniObjects *e,SfJniRef r,int global){
    if(!e||e->exception)return 0;
    if(!r)return 0;
    RefSlot *s=resolve(e,r);if(!s)return 0;
    int error=s->error;uint64_t id=s->exception_id;
    JString *string=s->string;
    SfJniRef copy=allocate(e,s->kind,global);
    if(copy){RefSlot *dst=&e->slots[(copy&255u)-1];dst->error=error;dst->exception_id=id;
        dst->string=string;if(string)string->refs++;}
    return copy;
}
int sf_jni_delete_ref(SfJniObjects *e,SfJniRef r,int global){
    if(!e)return -1;
    if(!r)return 0;
    RefSlot *s=resolve(e,r);if(!s)return -1;
    if(s->global!=!!global){fail(e,SF_JNI_BAD_TYPE);return -1;}
    release_slot(s);return 0;
}
void sf_jni_clear_locals(SfJniObjects *e){
    if(e)for(unsigned i=0;i<REF_COUNT;i++)if(!e->slots[i].global)release_slot(&e->slots[i]);
}
int sf_jni_exception(const SfJniObjects *e){return e?e->exception:SF_JNI_BAD_REF;}
void sf_jni_exception_clear(SfJniObjects *e){if(e){e->exception=0;e->exception_id=0;}}
SfJniRef sf_jni_context_call(SfJniObjects *e,SfJniRef r,const char *name,const char *descriptor){
    if(!e||e->exception)return 0;
    RefSlot *s=resolve(e,r);if(!s)return 0;
    if(s->kind!=1){fail(e,SF_JNI_BAD_TYPE);return 0;}
    if(!name||!descriptor||strcmp(name,"getAssets")||
       strcmp(descriptor,"()Landroid/content/res/AssetManager;")){
        fail(e,SF_JNI_BAD_METHOD);return 0;
    }
    return allocate(e,2,0);
}
AAssetManager *sf_jni_asset_manager(SfJniObjects *e,SfJniRef r){
    if(!e||e->exception)return 0;
    RefSlot *s=resolve(e,r);if(!s)return 0;
    if(s->kind!=2){fail(e,SF_JNI_BAD_TYPE);return 0;}
    return e->manager;
}
