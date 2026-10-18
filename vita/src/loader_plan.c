/* Integration contract, deliberately fail-closed. No Android instructions run. */
#include <stdio.h>
#include <stdint.h>
#include "elf_preflight.h"
int sf2_preflight(const char *path){
    ElfPreflight inspected;
    int rc=sf2_elf_inspect(path,&inspected);
    if(rc)return rc;
    return -4; /* Not implemented: ABI/import/JNI/renderer certification still required. */
}
/* Future verified so_util integration sequence:
 * 1 preflight ALL modules: machine, flags, relocations, TLS, address reservations.
 * 2 so_file_load dependencies at audited non-overlapping addresses.
 * 3 so_relocate; resolve every required import against typed tested adapters.
 * 4 independently reject missing imports (so_resolve returns 0 even if missing).
 * 5 so_flush_caches; ONLY THEN so_initialize after host services exist.
 * 6 JNI_OnLoad with real compatible JavaVM/JNIEnv tables, RegisterNatives registry.
 * 7 activity/surface/resume callbacks in discovered order; render/input/audio loop.
 * Never use so_resolve_with_dummy or call JNI_OnLoad(NULL,NULL).
 * ARMv8 ELF cannot be converted to ARMv7 by changing this header check.
 */
