# Original libmain JNI_OnLoad-only probe

The local 0.9-libmain1 build embeds the user's exact Special Edition 1.0.12 libmain.so, SHA-256 dad352d6cc2a8cf184d4e0a9dfe6d7f3768c329b4c684587c28e48eb01511d71. The optional SF2_LIBMAIN_PROBE_INPUT CMake input is empty by default. embed_libmain_probe.py refuses any other SHA. Generated game bytes, APKs and VPKs are excluded from publication; the binary was transferred only to the user's own Vita.

The existing executable mapper resolves the module before publication. A host data-only link using the real file verified all 30 external import names, 19 self definitions and relocated NativeLoader function pointers. Target addresses for host test imports were synthetic: this validates coverage/relocation, not full service compatibility. The target resolver uses the module's own visible exports first and an explicit 54-entry adapter scope next. The executable path still rejects TLS and INIT/INIT_ARRAY/PREINIT_ARRAY. This libmain has no initializer array, unlike Unity.

The probe configures an owner-thread JavaVM and a two-method immutable NativeLoader registration policy with ranges from the loaded executable image. It calls only JNI_OnLoad, expects JNI 1.6 and checks registered runtime pointers for load(String):boolean and unload():boolean. It does not call either registered method or start Unity. Registry/VM cleanup precedes module release. The exception-index table is bound only during the probe; whole C++ exception/unwind interoperability is not certified.

## Bounded runtime scope

libmain_runtime.c supplies typed entry-probe adapters, not a full Android libc/dynamic linker:

- Random stack guard from sceKernelGetRandomNumber and terminal abort/stack-check failure.
- 64-entry, synchronized DSO destructor registry, reverse registration order, callbacks marked consumed before invocation, safe callback reentrancy. Host test covers ownership selection, repeats, nested registration, capacity and reuse.
- Android ARM32 once candidate, numeric string conversion, bounded buffer formatting, byte-pattern search and base memory adapters.
- File API supports native-path read-only opens (plus inert close-on-exec flag on a platform without exec), read and close. Other Android open flags fail explicitly. Android /proc views are not implemented. errno remains the newlib per-thread location; numeric compatibility is only relied on for common initial-probe paths, not certified for the entire Android errno namespace.
- __sF provides only standard-stream tokens at ARM32 offsets 0/84/168, translated by fprintf/fflush. Disassembly of this exact libmain confirms stderr pointer arithmetic +168 in unwind diagnostic paths. Direct Android FILE field access, arbitrary FILE objects and a general stdio ABI are unsupported.
- dlopen/dlsym/dlclose report explicit unavailable-module-namespace errors if called. No successful handle or return is fabricated. These functions are not exercised by libmain JNI_OnLoad; NativeLoader.load remains intentionally uncalled until a real namespace and engine dependencies exist. dladdr recognizes only the mapped libmain range. The exception-index lookup recognizes only that module; host-frame/unwind interoperation remains unsupported.

All these restrictions are material. Linking every import name does not certify every behavior or enable running NativeLoader.load. The experiment advances the original JNI_OnLoad call only. Optional Google/ads behavior and DRM/licensing are not modified.

Local validation: 245 Python tests, 11 CTest targets (including concurrent once and destructor registry), warning-free Vita cross-build and VPK structural validation. Hardware JNI_OnLoad results are pending.

Build with the user's own extracted file:

```sh
cmake -S vita -B build-vita -DSF2_LIBMAIN_PROBE_INPUT=/absolute/path/to/libmain.so
cmake --build build-vita -j2
```

To disable embedding, reconfigure with -DSF2_LIBMAIN_PROBE_INPUT= (an existing build cache otherwise retains the path). Do not publish generated libmain_probe_image.h or the resulting game-containing VPK.


Hardware completion: 0.9-libmain2 executed original JNI_OnLoad successfully on Vita, returned JNI_VERSION_1_6 and registered both expected NativeLoader methods. See [hardware report](../reports/HARDWARE-0.9.md). NativeLoader.load/unload, Unity and IL2CPP were not invoked.
