# Constructor lifecycle planning

`tools/elf_lifecycle.py` reads ARM32 ET_DYN PT_DYNAMIC and REL metadata without
executing any instructions. It builds initialization order (DT_INIT followed
by DT_INIT_ARRAY order) and teardown order (reversed DT_FINI_ARRAY followed
by DT_FINI). Null/-1 unrelocated sentinels are skipped. Array entries require
local R_ARM_RELATIVE relocations; namespace-dependent entries are rejected.
All call targets must have valid ARM/Thumb alignment and lie inside file-backed
executable LOAD bytes. Array size/count/tag pairs and reservation bounds are
checked. Shared-object PREINIT calls are rejected.

Actual local libraries: Unity has 422 init calls and 2 fini calls; IL2CPP has
9 init calls and 2 fini calls. JSON plans retain offsets, slot indices and
hashes only. They contain no original instructions or assets. Runtime addresses
use a hypothetical base 0x90000000, not a device allocation.

Run:

```text
python tools/elf_lifecycle.py /local/libunity.so --out /local/unity-plan.json
```

This planner does not open the executable mapper's constructor gate. It does
not resolve all imports, initialize dependencies, provide TLS or execute code.
Before real constructors can run, the runtime needs complete required-import
resolution, a retained multi-module namespace, adequate executable memory for
Unity and IL2CPP together, ordered dependency initialization and owned teardown.
The existing 16 MiB sequential probe allocation is inadequate for IL2CPP's
39,088,128-byte load span and cannot serve as a simultaneous module arena.
A successful libmain JNI_OnLoad is not evidence that these constructors work.


## C planner and opt-in mapper

arm_lifecycle_plan now validates the source ELF together with the staged C
linker image. It checks singleton lifecycle tags, array tag pairs, bounds,
4-byte slot alignment, local R_ARM_RELATIVE records and actual relocated slot
values. Constructor/destructor targets must be ARM/Thumb aligned and wholly
inside file-backed executable LOAD bytes. Up to 1024 calls per phase are
supported. Nonzero PREINIT metadata is rejected. Calls through external symbol
relocations remain unsupported. Every error clears the output plan.

Initialization order is DT_INIT then forward DT_INIT_ARRAY; finalization is
reverse DT_FINI_ARRAY then DT_FINI. The entire teardown list is checked even
before initialization could begin. No function-pointer call occurs here.

arm_exec_map_planned is an explicit opt-in companion to arm_exec_map. It stages
and resolves the image, validates both lifecycle lists BEFORE publish/cache
sync, and returns an image plus immutable address plan. Missing imports, TLS,
source W+X, malformed lifecycle metadata, failed publish or failed cache sync
still reject the mapping. Failures release the reservation and clear plan/image.
The existing arm_exec_map retains its constructor rejection behavior. The Vita
diagnostic continues to use that existing path and does not enable this opt-in.
Image ready means mapped, not initialized; owners must not call exported code
before separately completing required dependency/runtime initialization.

## Actual-file cross-check

verify_lifecycle_c.py compares C-produced order against the Python planner on
original local libmain, Unity and IL2CPP. The C linker uses synthetic integer
import addresses in a HOST DATA BUFFER ONLY. These are not runtime stubs or
callable implementations. No executable mapping, constructors, JNI or Android
ARM instructions run in this audit. Import ABI is deliberately uncertified.
Reports contain only names, hashes, counts and pass/limit metadata:
libmain 0 init/2 fini; Unity 422 init/2 fini; IL2CPP 9 init/2 fini. All orders
match the independent Python path.

Twelve new tests cover opt-in publication order, unchanged default gate,
invalid last teardown entry, non-relative/unrelocated/mismatched slots,
duplicate/incomplete tags, limits/alignment, PREINIT, TLS, BSS targets and
publish/sync cleanup. Current validation: 291 Python tests and 14 CTest targets
pass, with VitaSDK cross-build/package validation. No new device result exists.
