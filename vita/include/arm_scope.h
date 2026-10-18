#ifndef SF_ARM_SCOPE_H
#define SF_ARM_SCOPE_H
#include "arm_link.h"
typedef struct {
    const char *provider,*name;
    uint32_t value;
    unsigned type,binding;
} ArmScopeEntry;
typedef struct {const ArmScopeEntry *entries;size_t count;} ArmSymbolScope;
/* Entries are already-filtered visible target definitions, in explicit lookup
   order. Provider/name strings and entries are borrowed, immutable, and must
   outlive scope use. No module loading or ABI certification is performed.
   Duplicate provider+name, unsupported type/binding and null function address
   are rejected. An empty scope is valid. Failure clears scope. */
int arm_scope_init(ArmSymbolScope *,const ArmScopeEntry *,size_t);
/* First matching definition wins, including weak definitions. A concrete
   function/object mismatch is an error, not permission to select a later one.
   type 0 means ELF NOTYPE: no signature/ABI inference can be made from it. */
int arm_scope_find(const ArmSymbolScope *,const char *,unsigned,
                   const ArmScopeEntry **selected);
/* ArmResolve-compatible adapter. Output address unchanged on miss/error. */
int arm_scope_resolve(const char *,unsigned,unsigned,uint32_t *,void *);
#endif
