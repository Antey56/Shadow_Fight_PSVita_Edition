# PS Vita hardware verification: 0.7-gfxtrace1

Date: 2026-10-18. Evidence: diagnostic log retrieved from the user's Vita over FTP after the user reported the test complete. The log identifies build 0.7-gfxtrace1. The transferred VPK was read back and matched SHA-256 `0fb2e0e0a0452f0e270b569ecd9344afa64a8548d58cb40da98d3783011946e9`; the running executable was not independently hashed.

## Observed results

- Core and selected JNI/memory/reflection/array startup probes: PASS.
- Controller sampling initialization returned 0. Independent START watcher creation and start succeeded.
- `BEFORE_VGL_INIT` at 187348 us; `AFTER_VGL_INIT resolution_fallback=0` at 3299029 us. Initialization returned after approximately 3.11 seconds.
- Main loop entered at 3342154 us. First input read returned one sample.
- First draw returned `gl_error=0x0000` at 4412986 us; first buffer swap returned at 4438374 us. Draw and swap also returned for traced frames 1 and 2.
- Touch DOWN/UP/MOVE events were recorded. Pointer IDs 1, 2 and 3 match the CROSS, SQUARE and TRIANGLE mappings; pointer ID 0 records directional movement. The log does not establish a CIRCLE test or distinguish stick from d-pad movement.
- `EXIT_REQUEST START main` at 28360356 us, followed by `BEFORE_EXIT_PROCESS frames=239` at 28376245 us. The normal main-loop START exit path was reached. Recovery from a stalled graphics thread via the watcher was not exercised.

## Memory observations

Kernel free bytes at entry: user 225443840, CDRAM 117440512, physically contiguous 27262976. After vitaGL initialization: user 15728640, CDRAM 0, physically contiguous 0; kernel query return code remained zero.

After the first swap, vitaGL reported these raw pool values:

| Pool ID | Free bytes | Total bytes |
| --- | ---: | ---: |
| 0 | 90055664 | 100663296 |
| 1 | 197604480 | 203423744 |
| 2 | 27262336 | 27262976 |
| 3 | 0 | 0 |
| 4 | 0 | 33554432 |

Kernel free memory and vitaGL pool free memory measure different allocation layers. Kernel CDRAM/physical free values of zero do not establish exhaustion: vitaGL has reserved pools with available space. Pool 4 values are preserved as reported, without interpreting them as a usable heap measurement.

## Scope and next gate

The earlier premature return after normal vitaGL initialization is fixed in the tested diagnostic path. The log establishes continued input, draw and swap execution. Visible screen contents were not independently observed in this test report, and these timings are not an FPS benchmark.

Unity was NOT started. This test does not certify Android ELF execution, ARM softfp interoperability, complete JNI semantics, EGL/audio integration or gameplay. The next engine gate remains a bounded, logged Unity bootstrap with required imports and ABI behavior validated before executing game code. The diagnostic's successful graphics loop is evidence for the diagnostic only.
