#ifndef SF_ELF_PREFLIGHT_H
#define SF_ELF_PREFLIGHT_H
#include <stdint.h>
typedef struct {
    uint32_t flags, load_min, load_max, load_count;
    int has_dynamic, has_tls, writable_executable;
} ElfPreflight;
/* Structural inspection only. Success never authorizes execution. */
int sf2_elf_inspect(const char *path,ElfPreflight *out);
const char *sf2_elf_error(int code);
#endif
