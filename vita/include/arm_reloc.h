#ifndef SF_ARM_RELOC_H
#define SF_ARM_RELOC_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint32_t offset, type, symbol_value;
    int symbol_resolved, thumb;
} ArmReloc;
/* Offsets are into an already validated contiguous image; values are target
   ARM32 addresses, NEVER host function pointers. Input must not alias image.
   Returns 0 after all writes, negative on error with image unchanged.
   Supported REL forms: NONE, ABS32, GLOB_DAT, JUMP_SLOT, RELATIVE(sym index 0).
   Resolver must reject missing required symbols before constructing this list. */
int arm_reloc_apply(unsigned char *image,size_t size,uint32_t bias,
                    const ArmReloc *rels,size_t count,size_t *failed_index);
#endif
