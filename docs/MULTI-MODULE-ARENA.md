# Multi-module ARM address arena

`arm_arena.c` supplies the existing ArmExecBackend interface over a borrowed
contiguous ARM32 backing reservation. It does not obtain executable memory.
The loader owner thread alone may mutate it. At most 64 image leases exist;
allocation uses page-rounded spans and absolute address alignment. First-fit
reuse naturally recovers adjacent released spans. No hardware guard pages or
per-segment permissions are provided by this allocator.

Cookies contain slot and generation. Stale cookies cannot publish, synchronize
or release a replacement lease. Exhausted generations retire rather than wrap.
Publication must cover the exact requested image span, then synchronization
must succeed before the existing mapper marks the image ready. Failed linking,
publication or synchronization releases only that image reservation. Releasing
an image must happen after all users and callbacks stop; cookies cannot revoke
raw function pointers already held by another subsystem.

The host C test reserves the real rounded spans of libmain (24,576), Unity
(13,893,632) and IL2CPP (39,088,128) in a simulated 64 MiB reservation, checks
alignment, exhaustion, fragmentation, stale handles, callback failures and
cleanup. Python integration tests map two authored ELF images through the
actual C mapper/backend, verify separate relocation values and unchanged first
image data, and exercise import/write/sync/capacity failure cleanup. No Android
instructions execute in these host tests.

## Vita backing candidate

`arm_arena_vita.c` cross-compiles against VitaSDK. An explicit caller must supply
valid user-callable kubridge imports for kuKernelMemProtect and
kuKernelFlushCaches from a compatible installed plugin. The component allocates
ordinary USER_RW backing memory (maximum 64 MiB), publishes staged images, then
requests RWX permissions and flushes caches. It does not resolve kernel addresses,
install/update plugins or probe unknown function pointers. It is compiled but
not called by the current diagnostic. Device executable-memory behavior remains
UNTESTED. This implementation makes no per-segment W^X claim.

Freeing the whole backing block is refused while any image lease remains.
A missing import, failed protection or failed allocation is a real error;
there is no silent successful fallback. The backing memory is separate from
vitaGL pools. Current diagnostic pool reservations cannot simply be kept at
maximum size while retaining another 64 MiB; engine memory budgets must be
reworked before combining this path with graphics.

The constructor/TLS gate in arm_exec_map remains closed. This memory component
does not implement dlopen namespaces, dependency initialization, JNI engine
lifecycle or the remaining required Android APIs.

Primary API references:
- https://github.com/bythos14/kubridge/blob/exceptions_mprotect/kubridge.h
- https://github.com/bythos14/kubridge/blob/exceptions_mprotect/exports.yml
- https://github.com/Rinnegatamante/so_util/blob/master/so_util.c

The fork header defines protection read/write/execute as 0x40/0x20/0x10.
The relevant NIDs are 0x566d2af1 (protect) and 0x38B70744 (flush).
These metadata values are not proof of installed plugin compatibility.
