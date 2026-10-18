# 0.8-elfsmoke1 hardware result and correction

Date: 2026-10-18. User completed installation/run and enabled FTP; the retrieved log identifies 0.8-elfsmoke1.

- ARM_MEMORY_PROBE PASS at 186376 us: selected byte-copy/move/fill/clear assembly operations passed on actual ARM hardware. This is not a full register-preservation/performance certification of all twelve helpers.
- OBJECT_ARRAY_PROBE PASS and core/JNI probes PASS.
- AUTHORED_ELF_EXEC FAIL rc=-20 at 213686 us, before entry selection/call. The required-import gate rejected the fixture import; no fixture ARM code ran.
- Graphics initialization returned; first three traced draws report GL error zero and swaps return. Recorded control events continue. START normal exit after 164 frames.

Cause: actual fixture dynsym marks sf_fixture_add as GLOBAL STT_NOTYPE. The resolver incorrectly accepted only STT_FUNC. The ELF linker preserves a supplied Thumb pointer for NOTYPE imports, but cannot infer a signature. The corrected fixture policy accepts NOTYPE or FUNC for this exact named and typed owned callback. Arbitrary untyped imports are not assumed ABI-compatible.

Correction validation: host mapping of the actual compiled fixture rechecked the NOTYPE import, its relocated JUMP_SLOT pointer and the RELATIVE data pointer. A regression test covers NOTYPE Thumb pointer preservation. 0.8-elfsmoke3 also uses C export lookup to locate the owned entry and logs any missing symbol. It was transferred and SHA-verified over FTP; a repeated hardware run is pending.

Neither Unity nor the game executed. A failed fixture probe is not reported as successful executable loading.


## 0.8-elfsmoke3 crash and VM-domain ordering fix

The user supplied a C2-12828-1 photo. The matching gzip-compressed core file was fetched from the user's Vita and kept local. The diagnostic log stops at BEFORE_AUTHORED_ELF_EXEC; prior ARM_MEMORY_PROBE, OBJECT_ARRAY_PROBE and core probes PASS.

The crashed main thread has PC 0xe00010e8 inside SceLibKernel, destination r0=0x83500000 and source r1=0x81504928. The fault registers report DFSR=0x82d and DFAR=0x83500000, consistent with a write permission fault at the VM block's start during ELF copying. This is before the authored entry is called. No Unity/game instruction ran.

Source inspection found sceKernelOpenVMDomain was incorrectly deferred until the cache-sync callback, after publish/memcpy. The corrected reserve path opens the domain before returning writable memory to publication; open failure frees the allocation. Cache sync remains after copying. This matches the VM-domain ordering used by so_util. Detailed markers now bracket reserve, open_before_write, copying, cache sync and each fixture entry return. 0.8-elfsmoke4 was built and transferred with SHA readback; its device result is pending.

Core register layout reference: [vita-parse-core](https://github.com/xyzz/vita-parse-core/blob/master/core.py). Raw dumps include unrelated system/application notes and are not published. Only the relevant addresses and diagnostic observations are recorded here.
