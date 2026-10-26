#ifndef SF_PRELOADED_DL_H
#define SF_PRELOADED_DL_H
#include "arm_namespace.h"
typedef struct {ArmNamespace *ns;char error[128];unsigned pending;} SfPreloadedDl;
/* Single owner thread, borrowed namespace. Does not load files or call code. */
void sf_preloaded_dl_init(SfPreloadedDl *,ArmNamespace *);
uint32_t sf_preloaded_dlopen(SfPreloadedDl *,const char *,int);
uint32_t sf_preloaded_dlsym(SfPreloadedDl *,uint32_t,const char *);
int sf_preloaded_dlclose(SfPreloadedDl *,uint32_t);
const char *sf_preloaded_dlerror(SfPreloadedDl *);
#endif
