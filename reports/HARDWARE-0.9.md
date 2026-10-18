# Original libmain entry probe: hardware results

## 0.9-libmain1

The authored ARM ELF executed successfully: first=11.75, second=7.50.
Original libmain was NOT executed. Its VM allocation failed with 0x80024B0B
(SCE_KERNEL_ERROR_MEMBLOCK_OVERFLOW in the installed VitaSDK header), after
the authored fixture allocation had been freed. This establishes an allocation
lifecycle problem; it does not establish a JNI or original-library failure.
No crash occurred. Graphics drew/swapped with GL error 0; START exited after
101 frames. Entry and pre-graphics user free memory were both 225443840 bytes.

## 0.9-libmain2: hardware PASS

Sequential probes lease the same process-local 16 MiB VM block instead of
allocating another block after releasing the first. Only one lease may exist;
overlapping reservations fail. Each map still publishes its fresh staged image
and synchronizes the VM cache before entry. Release ends the lease; explicit
shutdown frees the block before vitaGL initialization and logs the result.
The diagnostic status now includes original JNI_OnLoad success/failure.

The private VPK was built and validated, transferred to the user's Vita, and
verified by full readback. SHA-256:
bc6c8eca13d497d5365948c34448d04845ab510005a720006eb40f371cf4cea0.
245 Python tests and 13 host CTest targets pass. These host checks do not
validate Vita kernel VM semantics; the sequential hardware run is required.

This is not a multi-module arena or a Unity boot implementation. The block is
reused only after all references to the previous probe have been released.
Unity and gameplay remain untested and unstarted. Raw logs remain local.

Hardware log confirms original JNI_OnLoad entered at 0x835010e5 and returned
JNI_VERSION_1_6 (0x00010006). NativeLoader.load registered at 0x83501145;
unload at 0x835011e1, both matching the relocated original-library offsets.
ORIGINAL_LIBMAIN_ONLOAD PASS rc=0. Neither registered method was invoked.
The block was reused, cache synchronization succeeded, final free returned 0,
and pre-graphics user memory returned to 225443840 bytes. Three traced frames
reported GL error 0; physical controls generated touch events; START exited
normally after 109 frames. No Unity or gameplay execution is claimed.


## 0.9-services1: hardware PASS

MATH_ABI_PROBE PASS: pow=8, ldexp=0.25, sqrtf=2.5, modf=-0.25,
integer=-3. MUTEX_ABI_PROBE PASS: normal lock/trylock/busy-destroy/unlock/
destroy returned 0,16,16,0,0; recursive calls all 0; errorcheck sequence
0,35,0,0. These selected single-thread checks do not certify the complete
math catalog, floating-point environment or concurrent Vita contention.
Authored ELF and original libmain JNI_OnLoad both passed again. VM cleanup
returned 0, free user memory restored to 225443840 bytes before graphics.
GL error 0 in the three traced frames; controls emitted touch events;
START exited normally after 143 frames. Unity remains unstarted.
