# Vita diagnostic 0.7-objarrays2: hardware result

Date: 2026-10-18. Evidence: diagnostic log fetched over FTP after the user completed the test. The log header identifies 0.7-objarrays2. The VPK transfer was independently verified by full FTP readback: SHA-256 `8c460fbde064e496291d5ccf0b20bbe070445e90a7728565b71f48a5c9476e2e`, 500178 bytes. The running executable was not independently hashed.

- OBJECT_ARRAY_PROBE PASS at 181036 us: the diagnostic String-array construction, read, replacement with NULL, original-local deletion and retained-element checks passed on Vita.
- AFTER_CORE_PROBES: core=PASS, JNI_memory_reflection_arrays=PASS.
- vitaGL initialization returned at 3316421 us, resolution_fallback=0, after BEFORE_VGL_INIT at 204649 us (approximately 3.112 seconds).
- Main loop entered at 3375937 us. Draws for frames 0, 1 and 2 returned gl_error=0x0000; each subsequent swap returned. First swap completed at 4473567 us.
- Recorded touch IDs 0..4 include directional motion and CROSS/SQUARE/TRIANGLE/CIRCLE mappings. Actions 5/6 with count=2 record pointer addition/removal during simultaneous input. Logical events alone do not establish the exact physical source of every directional event.
- EXIT_REQUEST START main at 26425012 us, followed by BEFORE_EXIT_PROCESS frames=211 at 26446380 us. Normal START exit was reached; independent watcher recovery from a graphics stall was not exercised.
- Kernel and vitaGL free/total memory values after initialization/first swap match the preceding gfxtrace1 report. Zero free kernel CDRAM/physical bytes reflect reserved vitaGL pools with available internal space, not established out-of-memory failure.

This verifies the selected startup object-array probe on ARM hardware, not every array case or full JNI compatibility. Nested object arrays, class initialization and worker JNI remain unsupported/incomplete. FatalError and aggregate string budget exhaustion were tested on the host, not deliberately triggered on Vita. The log does not independently prove visible screen contents or establish a performance benchmark.

Unity was NOT started. Android executable loading, ABI certification, engine EGL/audio lifecycle and gameplay remain unverified. No DRM or license checks were bypassed.
