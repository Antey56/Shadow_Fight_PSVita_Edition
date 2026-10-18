# Executable ARM mapping and export lookup

## Boundary

arm_exec_map stages validated ARM32 ET_DYN bytes through the existing linker, reserves an ARM32 runtime base with a platform backend, resolves relocations before publishing, synchronizes code caches and only then marks the image ready. Backend reserve/publish/sync/release ownership and failure paths are tested on the host. Required missing imports prevent publication. Cleanup occurs on publish/sync errors; release clears the image and is idempotent. The API expects an empty output structure; callers must release existing owned images before reusing it.

Entry lookup preserves ARM/Thumb state and restricts calls to file-backed executable PT_LOAD ranges, including instruction width/alignment checks. It validates range membership, not arbitrary function signatures. This first executable path rejects TLS, W+X source segments and INIT/INIT_ARRAY/PREINIT_ARRAY. It does not call constructors or implement Android dynamic-loader semantics. Version lookup does not make arbitrary game ELF executable-ready.

## Vita backend and authored test

The diagnostic reserves a 16 MiB VM-domain block, opens the VM domain before writing, copies the staged image, synchronizes caches, calls only the embedded authored test entry and releases its allocation after return. VM-domain opening is process-wide and is not reverted by this test; no per-segment W^X claim is made. This backend does not use kubridge and is limited to a 16 MiB image reservation. It cannot by itself accommodate the complete Unity/IL2CPP runtime.

The fixture is compiled from tests/arm_smoke_fixture.c with softfp base AAPCS, Thumb, no CRT, hidden local data and a single exported entry. A generated header embeds only this authored ELF. There is one R_ARM_RELATIVE data pointer and one JUMP_SLOT import. The import is an owned base-AAPCS float-add callback. Expected entry returns: 11.75 then 7.50, checking callback argument/result convention, local relocated pointer and state/BSS persistence. The diagnostic START watcher is started before attempting ELF execution.

0.8-elfsmoke1 was transferred to the user's Vita with full SHA readback and remains pending hardware test. It selects the fixture entry from validated build metadata. Current source/build 0.8-elfsmoke2 additionally discovers that entry through the new C export lookup and compares it against build metadata. The constructor/runtime gate for actual game libraries remains closed. No original Android instructions have been executed by this project yet.

## Export lookup

arm_symbol_find uses PT_DYNAMIC, SysV DT_HASH symbol count, bounds-checked symbol/string tables and global/weak default/protected definitions. ARM/Thumb values and modular load bias are preserved. Default symbol versions are checked against bounded VERDEF/aux chains; nondefault/local versions are skipped. Explicit dlvsym-style selection, GNU-hash-only images, IFUNC, TLS and full Android namespace/version binding are not implemented. Imported symbol version requirements are not certified by this lookup.

The C lookup was exercised on the actual local libmain/libunity/libil2cpp byte buffers without loading their instructions. It recovered JNI_OnLoad/Unload exports and il2cpp_init where present. A real issue discovered by this check was Unity's JNI_OnLoad@@libunity default version; silently rejecting all version indices above one would reject that entry. Safe metadata results: reports/special-1.0.12/c-export-lookup.json.

## Evidence

245 Python tests and 10 CTest targets pass. New tests cover backend call order, failed reserve/link/publish/sync cleanup, bounds, stale ready state, modular load bias, constructor/TLS gates, malformed export/hash/name/version records, weak/hidden/local/absolute definitions and executable alignment. These host tests never execute Android ARM code. Vita cross-build/package validation pass; hardware AUTHORED_ELF_EXEC results are still required.

References: [VitaSDK sysmem APIs](https://docs.vitasdk.org/group__SceSysmemUser.html), [so_util VM-domain usage](https://github.com/Rinnegatamante/so_util/blob/master/so_util.c), [ELF symbol version structures](https://refspecs.linuxfoundation.org/LSB_3.1.1/LSB-Core-generic/LSB-Core-generic/symversion.html).


Hardware update: [0.8-elfsmoke1](../reports/HARDWARE-0.8.md) reached the required-import rejection gate (rc=-20); fixture execution did not occur. NOTYPE policy was corrected and 0.8-elfsmoke3 transferred for a repeated test. The earlier pending description above is historical for elfsmoke1; successful executable ARM hardware validation remains pending.


0.8-elfsmoke3 reached copying but crashed on the first VM write because the domain was opened too late. The 0.8-elfsmoke4 backend fixes the order and adds per-stage trace markers; see the hardware report. Successful entry execution is not claimed until the repeated test returns the expected floats.
