# Engine bootstrap: current boundary

Original libmain JNI_OnLoad executes successfully on real Vita in
0.9-libmain2 and 0.9-services1. It returns JNI_VERSION_1_6 and registers the
expected NativeLoader load/unload methods. Those methods have not been called.
Unity and Shadow Fight gameplay have not started. Hardware evidence:
[HARDWARE-0.9](../reports/HARDWARE-0.9.md).

The executable mapper and VM smoke test are hardware-verified for the authored
fixture; original libmain is mapped with a restricted service catalog. The
16 MiB VM block is reused between sequential probes and freed before graphics.
[Entry-probe limits](LIBMAIN-ENTRY-PROBE.md) remain applicable: dlopen/dlsym/
dlclose report unavailable rather than pretending to load Unity.

Selected math ABI and mutex checks, core JNI/reflection/array checks and
RTABI memory helper probes pass on Vita. Those tests do not certify the full
Android ABI, all math functions, concurrent thread lifecycle or engine APIs.

The next implemented building blocks are a tested multi-image address arena
and an uncalled Vita backing candidate using explicit compatible kubridge
imports. See [memory ownership and device limits](MULTI-MODULE-ARENA.md).
The data-only lifecycle planner validates 422 Unity and 9 IL2CPP initializer
addresses and their order; [constructor planning](ELF-LIFECYCLE.md) does not
enable the mapper's constructor gate or execute those functions.

The static gate inventory still contains 451 unique uncertified required
imports across the APK libraries. Its baseline records 372 required external
imports for Unity and 265 for IL2CPP, with overlap; these are not counts of
remaining functions after candidate adapters. A candidate name is not a
certified ABI implementation. Required missing imports continue to block mapping.

Remaining engine work includes a retained module namespace with real dlopen/
dlsym/dlclose ownership, complete required-import resolution, constructor and
teardown lifecycle, sufficient executable memory, bionic thread/TLS contracts,
Unity EGL/surface/audio lifecycle and IL2CPP metadata/assets. The diagnostic
vitaGL context does not implement Unity's EGL lifecycle.

Validation at this checkpoint: 259 Python tests, 14 CTest targets, VitaSDK
cross-build and VPK validation pass. New backing allocation/permissions are
not hardware-tested. No game binaries, APKs, generated game headers or device
dumps are published. No DRM or license checks are bypassed.
