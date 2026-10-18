#ifndef SF_NATIVE_REGISTRY_H
#define SF_NATIVE_REGISTRY_H
#include <stddef.h>
typedef struct { const char *class_name,*name,*descriptor; void (*address)(void); } NativeBinding;
typedef struct { NativeBinding entries[128]; size_t count; } NativeRegistry;
typedef struct { const char *class_name,*name,*descriptor; } NativeDeclaration;
/* Caller owns strings and must keep them alive. No JNI table layout is implied. */
int native_bind(NativeRegistry *,const char *,const char *,const char *,void (*)(void));
void (*native_lookup(const NativeRegistry *,const char *,const char *,const char *))(void);
/* Internal transaction, NOT a JNIEnv RegisterNatives entrypoint. Borrowed strings.
   Reject undeclared methods (-4); commit all bindings or leave registry intact.
   Trusted caller supplies valid pointers and serializes access. No ABI checking. */
int native_register_checked(NativeRegistry *,const NativeDeclaration *,size_t,
                            const NativeBinding *,size_t);
#endif
