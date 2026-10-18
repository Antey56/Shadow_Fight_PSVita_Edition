#ifndef SF_BIONIC_MEMORY_H
#define SF_BIONIC_MEMORY_H
#include <stddef.h>
#include "android_abi.h"
#include "arm_scope.h"
SF_ANDROID_ABI void *sf_bionic_malloc(size_t);
SF_ANDROID_ABI void *sf_bionic_calloc(size_t,size_t);
SF_ANDROID_ABI void *sf_bionic_realloc(void *,size_t);
SF_ANDROID_ABI void sf_bionic_free(void *);
SF_ANDROID_ABI void *sf_bionic_memcpy(void *,const void *,size_t);
SF_ANDROID_ABI void *sf_bionic_memmove(void *,const void *,size_t);
SF_ANDROID_ABI void *sf_bionic_memset(void *,int,size_t);
SF_ANDROID_ABI int sf_bionic_memcmp(const void *,const void *,size_t);
SF_ANDROID_ABI void *sf_bionic_memchr(const void *,int,size_t);
SF_ANDROID_ABI size_t sf_bionic_strlen(const char *);
SF_ANDROID_ABI int sf_bionic_strcmp(const char *,const char *);
SF_ANDROID_ABI int sf_bionic_strncmp(const char *,const char *,size_t);
SF_ANDROID_ABI char *sf_bionic_strchr(const char *,int);
SF_ANDROID_ABI char *sf_bionic_strrchr(const char *,int);
SF_ANDROID_ABI char *sf_bionic_strstr(const char *,const char *);
SF_ANDROID_ABI char *sf_bionic_strcpy(char *,const char *);
SF_ANDROID_ABI char *sf_bionic_strncpy(char *,const char *,size_t);
SF_ANDROID_ABI char *sf_bionic_strcat(char *,const char *);
SF_ANDROID_ABI char *sf_bionic_strncat(char *,const char *,size_t);
SF_ANDROID_ABI char *sf_bionic_strdup(const char *);
/* Target addresses only. Never mix these allocation/free families with
   unrelated heaps. Same standard C preconditions as underlying operations. */
int sf_bionic_memory_catalog(ArmScopeEntry *,size_t);
#endif
