# Additional Unity service candidates

## Mathematics

bionic_math.c has 48 typed base-AAPCS math entry points over the SDK math library. Each float/double signature is explicit, including int exponents and output pointers for modf/modff. An address catalog is available on ARM32 only. The candidate object was separately cross-compiled with the hard-float SDK and Android entry attributes. Reviewed disassembly of sqrtf, pow(double,double), ldexp(double,int) and modff shows the expected integer/VFP register moves around SDK calls.

Host numeric tests cover ordinary results, rounding, signed zero, NaN/infinity, negative-domain sqrt, remainder versus fmod, and output pointers. This is not full fenv/errno/subnormal/rounding-mode certification. The candidates are not yet wired into a Unity module namespace, and no claim is made that Unity has exercised them. Required-name matches are recorded in math-candidates.json; the engine certification count is unchanged.

## ARM32 mutex representation

bionic_mutex.c models the Android ARM32 4-byte control and uses a registry of up to 512 native SDK pthread mutexes. Public storage holds a generation-tagged identity, never a native host/Vita struct cast. Static normal (0), recursive (0x4000) and error-check (0x8000) initializers are recognized. Attribute APIs use explicit Android type numbers 0/1/2 mapped to native constants. Native error codes are translated for the supported operations; unknown errors become Android ENOTSUP.

Registry operations are synchronized. Leases prevent destruction during an active API operation; held-depth accounting prevents deleting a locked mutex even on native implementations that do not reject it reliably. Generation checks and owner-storage addresses reject stale or copied controls. Explicit init does not read uninitialized caller storage. Destroyed storage becomes invalid until explicit reinitialization. Generation exhaustion retires a slot rather than wrapping it.

Host tests use eight native threads and 40000 protected increments, recursive/error-check behavior, busy destroy/trylock, explicit initialization/attributes, destruction/reuse and invalid states. The ARM target object also compiles warning-free. The component is not yet wired into the Vita probe or the game resolver; linking it will require the SDK pthread library and its applicable source/relinking notices. No hardware mutex certification is claimed.

Process-shared/robust mutexes, timed mutex operations, condition variables, cancellation, fork and direct private bionic fast paths are unsupported. Condition-variable integration must share native mutex ownership/lease accounting; passing the 4-byte Android control to SDK cond_wait is invalid. JNI worker attachment remains a separate missing runtime feature.

Current local checks: 245 Python tests and 13 CTest targets pass. The pending hardware binary 0.9-libmain1 predates these isolated candidates and is still an original JNI_OnLoad-only probe, not a Unity/game build.

Reference for Android constants/storage: [AOSP bionic pthread.h](https://android.googlesource.com/platform/bionic/+/refs/heads/main/libc/include/pthread.h).


Hardware probe 0.9-services1 links these candidates into the Vita diagnostic.
Volatile numeric inputs exercise double pow, double/int ldexp, float sqrtf,
and pointer-output modf through their base-AAPCS declarations. Logged values
are compared against exact representable expectations (8, 0.25, 2.5, -0.25,
-3). Mutex probes exercise lazy normal initialization, trylock EBUSY,
busy destruction, recursive depth, errorcheck EDEADLK, unlock and cleanup.
The independent START watcher starts before the probes. This is a single-thread
hardware semantic probe; the host contention test does not establish Vita
contention behavior. Full math catalog, fenv and engine integration remain
uncertified. The VPK also repeats original libmain JNI_OnLoad without invoking
NativeLoader.load. Device results are pending.
