#define _POSIX_C_SOURCE 200809L
#include "android_asset_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
struct AAssetManager {char root[1024];size_t buffer_limit;};
struct AAsset {FILE *file;int64_t length,position;size_t buffer_limit;unsigned char *buffer;int broken;};
static int safe_name(const char *name){
    if(!name||!*name||name[0]=='/'||strlen(name)>1023)return 0;
    const char *component=name;
    for(const char *p=name;;p++){
        unsigned char c=(unsigned char)*p;
        if(c=='\\'||c==':'||(c&&c<32)||c==127)return 0;
        if(c=='/'||!c){
            size_t n=(size_t)(p-component);
            if(!n||(n==1&&component[0]=='.')||(n==2&&component[0]=='.'&&component[1]=='.'))return 0;
            if(!c)return 1;
            component=p+1;
        }
    }
}
AAssetManager *sf_asset_manager_create(const char *root,size_t limit){
    if(!root||!*root||strlen(root)>1023||limit>64u*1024u*1024u)return 0;
    AAssetManager *m=malloc(sizeof(*m));if(!m)return 0;
    strcpy(m->root,root);m->buffer_limit=limit;return m;
}
void sf_asset_manager_destroy(AAssetManager *m){free(m);}
SF_ANDROID_ABI AAsset *AAssetManager_open(AAssetManager *m,const char *name,int mode){
    if(!m||!safe_name(name)||mode<0||mode>3)return 0;
    char path[2050];int n=snprintf(path,sizeof path,"%s/%s",m->root,name);
    if(n<0||(size_t)n>=sizeof path)return 0;
    FILE *f=fopen(path,"rb");if(!f)return 0;
    struct stat st;
    if(fstat(fileno(f),&st)||!S_ISREG(st.st_mode)||fseek(f,0,SEEK_END)){fclose(f);return 0;}
    long length=ftell(f);
    /* Consistent initial backend limit even on hosts with 64-bit long. */
    if(length<0||(uint64_t)length>INT32_MAX||fseek(f,0,SEEK_SET)){fclose(f);return 0;}
    AAsset *a=calloc(1,sizeof(*a));if(!a){fclose(f);return 0;}
    a->file=f;a->length=length;a->buffer_limit=m->buffer_limit;return a;
}
SF_ANDROID_ABI int AAsset_read(AAsset *a,void *buf,size_t count){
    if(!a||a->broken)return -1;
    if(!count)return 0;
    if(!buf)return -1;
    if(a->position==a->length)return 0;
    if(count>(size_t)(a->length-a->position))count=(size_t)(a->length-a->position);
    if(count>INT_MAX)count=INT_MAX;
    size_t n=fread(buf,1,count,a->file);a->position+=(int64_t)n;
    return !n&&ferror(a->file)?-1:(int)n;
}
SF_ANDROID_ABI int64_t AAsset_seek64(AAsset *a,int64_t offset,int whence){
    if(!a||a->broken)return -1;
    int64_t base;
    if(whence==SEEK_SET)base=0;
    else if(whence==SEEK_CUR)base=a->position;
    else if(whence==SEEK_END)base=a->length;
    else return -1;
    if(offset < -base || offset > a->length-base)return -1;
    int64_t next=base+offset;
    if(fseek(a->file,(long)next,SEEK_SET))return -1;
    a->position=next;return next;
}
SF_ANDROID_ABI int32_t AAsset_seek(AAsset *a,int32_t offset,int whence){return (int32_t)AAsset_seek64(a,offset,whence);}
SF_ANDROID_ABI int64_t AAsset_getLength64(AAsset *a){return a?a->length:-1;}
SF_ANDROID_ABI int32_t AAsset_getLength(AAsset *a){return (int32_t)AAsset_getLength64(a);}
SF_ANDROID_ABI int64_t AAsset_getRemainingLength64(AAsset *a){return a?a->length-a->position:-1;}
SF_ANDROID_ABI int32_t AAsset_getRemainingLength(AAsset *a){return (int32_t)AAsset_getRemainingLength64(a);}
SF_ANDROID_ABI const void *AAsset_getBuffer(AAsset *a){
    if(!a||a->broken)return 0;
    if(a->buffer)return a->buffer;
    if(!a->buffer_limit||(uint64_t)a->length>a->buffer_limit)return 0;
    unsigned char *buffer=malloc(a->length?(size_t)a->length:1);if(!buffer)return 0;
    if(fseek(a->file,0,SEEK_SET)){free(buffer);return 0;}
    size_t n=fread(buffer,1,(size_t)a->length,a->file);
    int failed=n!=(size_t)a->length||ferror(a->file);
    if(fseek(a->file,(long)a->position,SEEK_SET)){a->broken=1;failed=1;}
    if(failed){free(buffer);return 0;}
    a->buffer=buffer;return buffer;
}
SF_ANDROID_ABI int AAsset_isAllocated(AAsset *a){return a&&a->buffer!=0;}
SF_ANDROID_ABI void AAsset_close(AAsset *a){if(a){fclose(a->file);free(a->buffer);free(a);}}
int sf_asset_target_catalog(ArmScopeEntry *out,size_t cap){
#if defined(SF2_VITA_TARGET) && defined(__arm__)
    if(!out||cap<11||sizeof(uintptr_t)!=4)return -22;
#define ENTRY(n) (ArmScopeEntry){"host:libandroid:assets",#n,(uint32_t)(uintptr_t)&n,2,1}
    out[0]=ENTRY(AAssetManager_open);out[1]=ENTRY(AAsset_read);out[2]=ENTRY(AAsset_seek);
    out[3]=ENTRY(AAsset_seek64);out[4]=ENTRY(AAsset_getLength);out[5]=ENTRY(AAsset_getLength64);
    out[6]=ENTRY(AAsset_getRemainingLength);out[7]=ENTRY(AAsset_getRemainingLength64);
    out[8]=ENTRY(AAsset_getBuffer);out[9]=ENTRY(AAsset_isAllocated);out[10]=ENTRY(AAsset_close);
#undef ENTRY
    return 11;
#else
    (void)out;(void)cap;return -22;
#endif
}
