#ifndef SF_ARM_DYNAMIC_H
#define SF_ARM_DYNAMIC_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint32_t target_vaddr,type,symbol_index; } ArmRawRel;
typedef struct {
    uint32_t load_min,load_end,strtab_file_offset,strtab_size,symtab_vaddr;
    size_t relocation_count;
} ArmDynamicInfo;
/* Input file bytes only; never maps or executes Android code.
   First call with out=NULL/capacity=0 to obtain relocation_count.
   Then provide that many entries. On any error info is zeroed; output entries
   may be partially populated and MUST be discarded. No image writes occur.
   File bytes must remain unchanged between calls. */
int arm_dynamic_read(const unsigned char *file,size_t size,ArmRawRel *out,
                     size_t capacity,ArmDynamicInfo *info);
#endif
