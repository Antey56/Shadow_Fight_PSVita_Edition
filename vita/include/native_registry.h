#ifndef SF_NATIVE_REGISTRY_H
#define SF_NATIVE_REGISTRY_H
#include <stddef.h>
typedef struct { const char *class_name,*name,*descriptor; void (*address)(void); } NativeBinding;
typedef struct { NativeBinding entries[128]; size_t count; } NativeRegistry;
/* Caller owns strings and must keep them alive. No JNI table layout is implied. */
int native_bind(NativeRegistry *,const char *,const char *,const char *,void (*)(void));
void (*native_lookup(const NativeRegistry *,const char *,const char *,const char *))(void);
#endif
