#ifndef SF_ANDROID_ASSET_ADAPTER_H
#define SF_ANDROID_ASSET_ADAPTER_H
#include <stddef.h>
#include <stdint.h>
#include "android_abi.h"
#include "arm_scope.h"
typedef struct AAssetManager AAssetManager;
typedef struct AAsset AAsset;
/* Root is a trusted, immutable extracted assets directory. Rejects lexical
   traversal; NOT an OS sandbox or protection against symlinks/reparse races.
   Per-asset buffer limit 0 disables getBuffer; maximum is 64 MiB.
   Manager is immutable after creation, assets have independent file cursors.
   The caller must not concurrently use/close the same asset handle. */
AAssetManager *sf_asset_manager_create(const char *root,size_t buffer_limit);
void sf_asset_manager_destroy(AAssetManager *);
int sf_asset_target_catalog(ArmScopeEntry *,size_t);
SF_ANDROID_ABI AAsset *AAssetManager_open(AAssetManager *,const char *,int);
SF_ANDROID_ABI int AAsset_read(AAsset *,void *,size_t);
/* Explicit ARMv7 Android widths, independent of host off_t/long widths. */
SF_ANDROID_ABI int32_t AAsset_seek(AAsset *,int32_t,int);
SF_ANDROID_ABI int64_t AAsset_seek64(AAsset *,int64_t,int);
SF_ANDROID_ABI int32_t AAsset_getLength(AAsset *);
SF_ANDROID_ABI int64_t AAsset_getLength64(AAsset *);
SF_ANDROID_ABI int32_t AAsset_getRemainingLength(AAsset *);
SF_ANDROID_ABI int64_t AAsset_getRemainingLength64(AAsset *);
SF_ANDROID_ABI const void *AAsset_getBuffer(AAsset *);
SF_ANDROID_ABI int AAsset_isAllocated(AAsset *);
SF_ANDROID_ABI void AAsset_close(AAsset *);
#endif
