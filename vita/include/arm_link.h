#ifndef SF_ARM_LINK_H
#define SF_ARM_LINK_H
#include <stddef.h>
#include <stdint.h>
/* Resolver returns 1 with target ARM32 value, 0 if absent, negative on error.
   It must implement the intended symbol lookup scope and must not execute
   Android code. Addresses are target integers, never host function pointers. */
typedef int (*ArmResolve)(const char *name,unsigned binding,unsigned type,
                         uint32_t *value,void *context);
typedef struct { unsigned char *bytes; size_t size; uint32_t vaddr_min,bias; } ArmLinkedImage;
typedef struct { int code; size_t relocation_index; char missing_symbol[128]; } ArmLinkError;
/* Produces DATA ONLY. Caller frees result.bytes. Output is zero on failure.
   No constructors, cache operations, executable mappings or JNI are invoked.
   Current module definitions are used for local/hidden/protected symbols;
   global default-visible definitions require resolver selection too.
   Missing undefined weak references become zero. No automatic external stubs. */
int arm_link_image(const unsigned char *file,size_t file_size,uint32_t runtime_start,
                   size_t max_image,ArmResolve resolver,void *context,
                   ArmLinkedImage *result,ArmLinkError *error);
void arm_link_dispose(ArmLinkedImage *result);
#endif
