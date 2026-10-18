# ARM32 once initializer candidate

sf_bionic_pthread_once accepts 4-byte aligned, initially zero Android ARM32 once storage. GCC acquire/release atomics implement states 0 (uninitialized), 1 (running), 2 (complete). Only the thread winning the compare/exchange invokes the base-AAPCS callback. Waiters observe completion before reading initialized data. Waiting yields/sleeps on the host or delays a Vita thread rather than replacing another runtime's pthread_once structure.

This is a candidate service, not full pthread support. Fork generation, thread cancellation and initializer unwind/longjmp recovery are unsupported. No support is claimed for process-shared storage or unknown nonzero state bits. Recursive initialization is not made recoverable. JNI worker attachment remains unsupported separately. The function is not yet published as a certified game resolver import.

Host CTest starts eight native threads, each repeatedly requesting the same initialization. Exactly one initializer call and consistent published payload are asserted after joining all workers. NULL/misaligned-contract inputs and unknown state are rejected with Android EINVAL (22). The implementation is cross-compiled with the Vita target; no multi-thread hardware certification is claimed.

Reference: [Android bionic pthread_once implementation](https://android.googlesource.com/platform/bionic/+/refs/heads/main/libc/bionic/pthread_once.cpp).
