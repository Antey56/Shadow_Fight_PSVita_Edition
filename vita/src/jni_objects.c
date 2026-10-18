#include "jni_objects.h"
#include <stdlib.h>
#include <string.h>
#define REF_COUNT 64
#define GEN_MAX 0xffffffu
typedef struct {uint32_t generation;unsigned char kind,global;} RefSlot;
struct SfJniObjects {AAssetManager *manager;RefSlot slots[REF_COUNT];int exception;};
static void fail(SfJniObjects *e,int error){if(e&&!e->exception)e->exception=error;}
static RefSlot *resolve(SfJniObjects *e,SfJniRef r){
    unsigned index=r&255u;uint32_t gen=r>>8;
    if(!e||!index||index>REF_COUNT||!gen){fail(e,SF_JNI_BAD_REF);return 0;}
    RefSlot *s=&e->slots[index-1];
    if(!s->kind||s->generation!=gen){fail(e,SF_JNI_BAD_REF);return 0;}
    return s;
}
static SfJniRef allocate(SfJniObjects *e,unsigned kind,int global){
    if(!e||e->exception)return 0;
    for(unsigned i=0;i<REF_COUNT;i++){
        RefSlot *s=&e->slots[i];
        if(!s->kind&&s->generation<GEN_MAX){
            s->generation++;s->kind=(unsigned char)kind;s->global=(unsigned char)!!global;
            return (s->generation<<8)|(i+1);
        }
    }
    fail(e,SF_JNI_CAPACITY);return 0;
}
int sf_jni_kind(SfJniObjects *e,SfJniRef r){
    if(!e||e->exception)return 0;
    RefSlot *s=resolve(e,r);return s?s->kind:0;
}
SfJniRef sf_jni_class(SfJniObjects *e,int kind){
    if(kind!=3&&kind!=4){fail(e,SF_JNI_BAD_TYPE);return 0;}
    return allocate(e,(unsigned)kind,0);
}
void sf_jni_error(SfJniObjects *e,int error){fail(e,error);}
SfJniObjects *sf_jni_objects_create(const char *root,size_t limit){
    SfJniObjects *e=calloc(1,sizeof(*e));if(!e)return 0;
    e->manager=sf_asset_manager_create(root,limit);
    if(!e->manager){free(e);return 0;}return e;
}
void sf_jni_objects_destroy(SfJniObjects *e){if(e){sf_asset_manager_destroy(e->manager);free(e);}}
SfJniRef sf_jni_context(SfJniObjects *e){return allocate(e,1,0);}
SfJniRef sf_jni_new_ref(SfJniObjects *e,SfJniRef r,int global){
    if(!e||e->exception)return 0;
    if(!r)return 0;
    RefSlot *s=resolve(e,r);return s?allocate(e,s->kind,global):0;
}
int sf_jni_delete_ref(SfJniObjects *e,SfJniRef r,int global){
    if(!e)return -1;
    if(!r)return 0;
    RefSlot *s=resolve(e,r);if(!s)return -1;
    if(s->global!=!!global){fail(e,SF_JNI_BAD_TYPE);return -1;}
    s->kind=0;return 0;
}
void sf_jni_clear_locals(SfJniObjects *e){
    if(e)for(unsigned i=0;i<REF_COUNT;i++)if(!e->slots[i].global)e->slots[i].kind=0;
}
int sf_jni_exception(const SfJniObjects *e){return e?e->exception:SF_JNI_BAD_REF;}
void sf_jni_exception_clear(SfJniObjects *e){if(e)e->exception=0;}
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
