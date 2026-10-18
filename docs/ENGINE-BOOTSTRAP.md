# Engine bootstrap: actual current boundary

As of 2026-10-18, only authored diagnostic code has run on Vita. Unity and Shadow Fight have not run. There is no evidence supporting a same-day gameplay promise. Tests of the compatibility model must not be described as engine execution.

## Actual libmain entry

The supplied Special Edition 1.0.12 libmain.so SHA-256 is dad352d6cc2a8cf184d4e0a9dfe6d7f3768c329b4c684587c28e48eb01511d71. Reviewed Thumb instructions show JNI_OnLoad (pointer 0x10e5) using JavaVM AttachCurrentThread (slot 4), finding com/unity3d/player/NativeLoader and registering two methods from table 0x5004: load(String):boolean at Thumb pointer 0x1145 and unload():boolean at 0x11e1. These are ELF virtual pointers, never direct Vita call addresses.

The load path references libunity.so and then libil2cpp.so through the loader helper at 0x1290. This helper builds a path, calls dlopen with a fallback bare library name, looks up JNI_OnLoad with dlsym and calls it through a function pointer. Native unload similarly looks up JNI_OnUnload before dlclose. The full error/lifetime behavior must be preserved when replacing Android's linker.

Previously the bridge's class allowlist omitted NativeLoader. The class is now available only when explicitly present in the immutable registration declaration policy. Its two verified records were replayed through the authored host JNI table with link-time pointers stored as integers; no original ARM instruction was executed. audit_libmain_bootstrap.py validates the exact ELF hash, key slot/count instructions, PC-relative strings and three relocated pointers per table entry against DEX declarations. It is a profile based on manual disassembly, not a general call-graph analyzer.

## ARM compiler memory helpers

Added __aeabi_memcpy/memmove/memset/memclr and their 4/8 alignment variants: twelve target resolver entries. ARM code uses only r0-r3 and condition flags, never calls libc or touches VFP state. This matters because the ARM runtime helper contract permits fewer clobbers than an ordinary C memcpy call; a base-AAPCS annotation alone would not preserve all VFP registers. memset's n,c parameter order differs from C memset. Aligned variants are aliases of the general byte implementation; counts need not be alignment multiples.

Host tests cover byte counts 0..63, boundary sentinels, fill truncation, forward/backward overlap and self-move. They run an independent C reference implementation, not the Thumb object. The target assembly was cross-built and inspected: only integer instructions and resolved local branches, no external calls. The diagnostic now includes ARM_MEMORY_PROBE, which has not yet been exercised on Vita. These byte loops prioritize correctness and are not performance-tuned.

[ARM RTABI memory-helper specification](https://github.com/ARM-software/abi-aa/blob/main/rtabi32/rtabi32.rst#memory-copying-clearing-and-setting).

## What still prevents engine execution

The current ELF linker creates a data buffer only. There is no executable ELF mapping, cache synchronization and validated ARM constructor/JNI_OnLoad execution path. libmain still needs real dlopen/dlsym/dlclose, unwind registration, bionic TLS/errno, once initialization and related runtime contracts; some paths also use Android FILE data and file APIs. Unity additionally needs many Android/pthread services and engine graphics/audio lifecycle. The diagnostic vitaGL context does not implement Unity's EGL surface lifecycle. IL2CPP introduces another runtime and metadata/asset initialization path.

The existing count of 451 unique uncertified required imports is unchanged: new adapter candidates are not automatically certified whole-engine ABI implementations. NativeLoader metadata support does not make load/unload execute engine code. Blind successful stubs or forcing a call to unresolved code would produce a crash rather than a playable port.

The nearest execution milestone is a validated executable-mapping/cache/ABI smoke test using authored ARM code, followed by completely resolved libmain and its real dynamic-loader contract. Unity JNI_OnLoad comes after that, with constructors and lifecycle dependencies accounted for. A successful menu frame and playable combat are separate later observations, not consequences of passing diagnostic probes.

Local validation: 217 Python tests, 9 CTest targets, Vita cross-build and VPK validation pass. Current source/build identity: 0.7-bootstrap1. Device-tested identity remains 0.7-objarrays2. APK, game libraries and assets are kept local; DRM/license checks are not bypassed.
