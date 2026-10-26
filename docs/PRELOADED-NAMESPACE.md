# Retained module registry and preloaded libdl adapter

The C ArmNamespace registry holds up to sixteen mapped ELF views. Registration
requires a ready mapping and matching dynamic-parser span/bias. Files, image
objects and mapping storage are borrowed immutable until removal. The registry
rejects duplicate names/images and overlapping runtime ranges. Each slot uses
a generation-tagged ARM32 handle; retired generations never wrap. One loader
owner thread alone may access this registry and its error state.

Dependencies must already be registered, are pinned for the lifetime of the
child and cannot be duplicated. This builds an acyclic explicit registration
graph; it does not discover DT_NEEDED or load files. A module can initialize
only after its declared dependencies reach READY. State transitions distinguish
LINKED, INITIALIZING, READY, FINALIZING, FAILED and FINISHED. The owner asserts
that actual initialization/teardown succeeded; transitions themselves execute
no instructions and do not prove constructors ran.

Opening increments references only for READY modules. Closing decrements them
but does not execute JNI_OnUnload, destructors or unmap anything. Removal and
finalization are blocked by open references or dependency pins. Failed/finished
modules are invisible to open; old handles cannot close a replacement module.
The owner must stop module users, perform actual teardown and remove the view
before releasing its ArmExecImage. The registry cannot revoke raw function
pointers retained elsewhere.

Symbol lookup uses the validated C ELF export parser with runtime load bias,
ARM/Thumb state and default symbol-version checks. An opened module searches
its own exports and then declared dependencies breadth-first, visiting each
handle once. Relocation resolution is a separate explicit global registration-
order scope over LINKED/INITIALIZING/READY modules. This scope does not implement
Android global/local groups or ELF symbolic namespace semantics.

## Adapter and runtime wiring

SfPreloadedDl supplies owner-thread dlopen/dlsym/dlclose/dlerror behavior for
this explicit preloaded registry. It returns generation handles and actual
relocated symbol values; no successful dummy addresses or fallback zero stubs
are generated. Errors are consumed once through dlerror. A valid zero-valued
absolute symbol is distinguishable from failure through the error state.

The libmain runtime's existing base-AAPCS wrappers now delegate to this adapter.
Call sf_libmain_runtime_namespace with a live prepared registry to opt in; the
current diagnostic never supplies one, so attempted library loading still
returns a real namespace-unavailable error. The registry must outlive the
adapter and all handles. Do not rebind/reset it with outstanding references.

Only exact registered basenames and Android ARM32 RTLD_NOW=0 / RTLD_LAZY=1
are accepted. Both require pre-resolved imports. GLOBAL/NOLOAD/NODELETE flags,
paths, NULL program handles, RTLD_DEFAULT/NEXT, dlvsym, filesystem/ZIP loading,
worker-thread calls and automatic constructors are unsupported and rejected.
There is no automatic dependency discovery or scope promotion.

Primary ABI reference for LP32 flags and libdl contracts:
https://github.com/aosp-mirror/platform_bionic/blob/master/libc/include/dlfcn.h

## Verification and remaining boundary

14 namespace tests exercise dependency pins/order, teardown, failed states,
references, stale handles, slot reuse/retirement, bounds, type mismatch, errors,
mapping overlap and breadth-first/diamond dependency search. Six adapter tests
cover resolved symbol values, ARM32 flags, real missing-module errors, invalid
paths/handles, missing namespaces, close semantics and zero-valued symbols.
Fixtures are authored ELF metadata; no game ARM instruction executes in tests.

279 Python tests and 14 CTest targets pass. VitaSDK cross-build and VPK
validation pass. These namespace changes are not device-tested and were not
transferred under an old diagnostic identity. Original libmain's last hardware
PASS remains services1. Unity initialization is still blocked by required-import
coverage, executable-memory validation and controlled constructor execution.

Exact original-libmain disassembly confirms movs r1,#1 immediately before both dlopen calls (instruction vaddrs 0x12e0 and 0x12ec). The hash-pinned bootstrap audit now revalidates those two instructions. Its flag value is accepted by the preloaded adapter; no NativeLoader.load execution is claimed.
