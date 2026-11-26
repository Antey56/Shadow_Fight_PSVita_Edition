# Whole-startup gate: exact Special Edition 1.0.12 binaries

This checkpoint evaluates the startup path together instead of inferring engine
readiness from individual smoke tests. unity_startup_gate.py requires the pinned
hashes of libmain/libunity/libil2cpp, reads actual REL/JMPREL references and
validates the reviewed Unity JNI_OnLoad call sites and registration patterns.
No Android instructions are executed by the tool. The engine remains BLOCKED.

## Corrected startup profile

Unity JNI_OnLoad directly calls a timing helper at 0x19fd98, attaches to JavaVM,
and calls nine reviewed registration helpers. The old profile covered only
three groups/28 methods. The expanded profile checks all nine groups/40 methods:
UnityPlayer 25, ChoreographerCallback 1, SwappyDisplayManager 2, GoogleARCoreApi 3,
Camera2Wrapper 4, HFPStatus 2, AudioVolumeHandler 1,
UnityCoreAssetPacksStatusCallbacks 1 and OrientationLockListener 1.

All forty registrations replay successfully through the host JNI bridge, with
function pointers stored only as integers. This does not provide camera, AR,
audio, asset-pack or other Java service behavior and never reports fake service
success. It does not bypass licenses or invoke optional Google services.

The direct Thumb BL targets, registration instruction forms, count, literals,
class strings and DEX/native table correspondence are checked. The startup
profile is still not a complete transitive call graph. The timing helper and
422 constructors require additional runtime behavior before JNI registration.

## Required imports: narrower and reproducible scope

The selected ELF relocation tables reference 30 required external names in
libmain, 316 in Unity and 232 in IL2CPP, plus one weak name in IL2CPP. Their
required union has 368 names. Full symbol-name lists are grouped by subsystem
in the JSON report. Weak and defined symbols are not counted as required
external relocation imports.

Earlier counts (451 across all APK libraries, 372 Unity/265 IL2CPP baseline
records) used a different, broader inventory. The change in count does NOT
mean those APIs were implemented. None of the new count is blanket-certified.
The linker still rejects missing required symbols. No placeholder addresses
from data-only audits are installed into a runtime mapping.

## A clock requirement before registration

Reviewed Unity clock call sites use Android IDs 1 and 7 (MONOTONIC and BOOTTIME)
and read a time32 timespec as two 32-bit fields. The reviewed sequence does not
check return values before using output, so merely returning an unsupported-
clock error is not a usable backend for executing this path.

android_clock.c now provides a typed base-AAPCS time32 adapter over an explicit
immutable provider. It validates clock masks, callback errors, nanosecond range
and signed-32 seconds overflow; failed calls leave output unchanged. It uses
Android errno values. No provider is bound by default, and the adapter is not
added to the Unity resolver. It must not alias BOOTTIME to MONOTONIC without
verified suspend semantics. A real Vita backend and suspend validation remain
required. This is a contract implementation, not certification of native clocks.

References:
https://github.com/aosp-mirror/platform_bionic/blob/master/libc/include/time.h
https://github.com/aosp-mirror/platform_bionic/blob/master/libc/include/sys/time.h

## Memory blockers and explicit experimental profile

Current diagnostic newlib heap: 32 MiB. IL2CPP staged load span: 39,084,628 bytes
(the rounded span is 39,088,128); heap is too small even without file input or
relocation work buffers. The report derives exact values from the ELF.
Current C linking transient estimate for IL2CPP is 88,998,236 bytes: file input
plus staged image plus 40 bytes per relocation (raw, resolved and transactional
write records). This excludes allocator overhead and other runtime allocations.

The diagnostic measured 225,443,840 free USER bytes before graphics, with an
existing 32 MiB heap. A proposed 64 MiB native arena plus the observed vitaGL
USER pool (203,423,744) exceeds that free memory by 45,088,768 bytes. The 16 MiB
VM backend still cannot serve as the full engine arena.

An OFF-by-default SF2_ENGINE_MEMORY_PROFILE CMake option now selects a 128 MiB
heap and computes a vitaGL threshold intended to cap its USER pool at 32 MiB.
Before graphics it requires room for a pending 64 MiB native reservation plus
16 MiB explicit headroom. The budget helper rejects underflow, invalid sizes
and insufficient space. This diagnostic branch does not allocate the native
arena itself. A future caller with an already allocated arena needs a different
pending-reservation allowance, not another 64 MiB deduction.

From the observed baseline this policy predicts 23 MiB USER headroom after the
heap, arena and graphics USER pool. That is an estimate, not a verified runtime
peak or evidence that game assets fit. The source threshold behavior was
checked against vitaGL; real allocator/GXM overhead still requires device logs.
https://github.com/Rinnegatamante/vitaGL/blob/master/source/vgl.c

Build default or explicitly opt in, with VitaSDK in the environment:

```text
cmake -S vita -B build-vita-engine-profile -G Ninja -DSF2_LIBMAIN_PROBE_INPUT= -DSF2_ENGINE_MEMORY_PROFILE=ON
cmake --build build-vita-engine-profile
```

Both default and experimental configurations cross-build and validate packages.
Neither rebuilt binary was transferred under the already-tested ctors1 identity.
The preserved hardware-tested artifact remains unchanged in artifacts/.

## Remaining hard gates

The installed kubridge large executable-memory path needs hardware validation.
Required libc/pthread/TLS/OS APIs remain incomplete. Original constructors and
Unity/IL2CPP JNI_OnLoad have not run. Unity EGL/surface/audio/Java lifecycle,
metadata/assets and playable combat remain unimplemented. Standard Android2Vita
guidance excludes Unity from its usual supported candidate path; this remains
an experimental engine-compatibility effort with no guaranteed playable result.

Validation: 305 Python tests, 16 CTest targets, forty native registrations replayed
on host, default and experimental VitaSDK builds/package checks. No Unity/game
execution or new hardware result is claimed at this checkpoint.
