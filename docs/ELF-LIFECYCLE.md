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
