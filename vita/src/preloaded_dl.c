#include "preloaded_dl.h"
#include <stdio.h>
#include <string.h>
static void error(SfPreloadedDl *dl,const char *message){
    if(dl){snprintf(dl->error,sizeof dl->error,"%s",message);dl->pending=1;}
}
static void namespace_error(SfPreloadedDl *dl){
    const char *message=arm_namespace_error(dl->ns);
    error(dl,message?message:"preloaded namespace operation failed");
}
void sf_preloaded_dl_init(SfPreloadedDl *dl,ArmNamespace *ns){
    if(dl){memset(dl,0,sizeof(*dl));dl->ns=ns;}
}
uint32_t sf_preloaded_dlopen(SfPreloadedDl *dl,const char *name,int flags){
    if(!dl)return 0;
    /* Android LP32: RTLD_NOW=0, RTLD_LAZY=1, RTLD_GLOBAL=2.
       Only fixed-scope preloaded lookup is supported, no flag side effects. */
    if(flags!=0&&flags!=1){error(dl,"unsupported ARM32 dlopen flags");return 0;}
    if(!dl->ns){error(dl,"preloaded module namespace unavailable");return 0;}
    uint32_t handle=0;
    if(arm_namespace_open(dl->ns,name,&handle)){namespace_error(dl);return 0;}
    return handle;
}
uint32_t sf_preloaded_dlsym(SfPreloadedDl *dl,uint32_t handle,const char *name){
    if(!dl)return 0;
    if(!dl->ns){error(dl,"preloaded module namespace unavailable");return 0;}
    ArmExport found;
    if(arm_namespace_symbol(dl->ns,handle,name,&found)!=1){namespace_error(dl);return 0;}
    return found.value;
}
int sf_preloaded_dlclose(SfPreloadedDl *dl,uint32_t handle){
    if(!dl)return -1;
    if(!dl->ns){error(dl,"preloaded module namespace unavailable");return -1;}
    if(arm_namespace_close(dl->ns,handle)){namespace_error(dl);return -1;}
    return 0;
}
const char *sf_preloaded_dlerror(SfPreloadedDl *dl){
    if(!dl||!dl->pending)return 0;
    dl->pending=0;return dl->error;
}
