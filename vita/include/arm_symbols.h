#ifndef SF_ARM_SYMBOLS_H
#define SF_ARM_SYMBOLS_H
#include <stddef.h>
#include <stdint.h>
typedef struct {uint32_t value,size;unsigned binding,type,absolute;} ArmExport;
/* Export lookup for validated ARM32 SysV-hash ELF. Does not execute code.
   Returns 1 found, 0 absent, negative malformed/unsupported. Default/protected
   global/weak definitions only; default versions checked against VERDEF.
   Nondefault versions skipped; explicit-version lookup and IFUNC unsupported. */
int arm_symbol_find(const unsigned char *,size_t,const char *,uint32_t,ArmExport *);
#endif
