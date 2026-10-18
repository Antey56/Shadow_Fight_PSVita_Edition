# Hardware report: diagnostic 0.6

Source: user-provided report and VitaShell photographs, 2026-10-18. Raw log was transcribed by the user, not independently downloaded. Device: original PS Vita, VitaShell 2.02; libshacccg.suprx installed.

- Original VPK installation failed with 0x8010113D. User converted the three RGBA LiveArea PNGs to indexed PNG8 and repacked, reporting no eboot/SFO/XML changes. Installation then succeeded.
- App reached the vitaGL splash, but the application's own screen did not appear. Buttons/START were not tested during that run.
- Transcribed log reports adapter check i=7 d=1.25, core=PASS and JNI/memory/reflection/arrays probe=PASS. Last probe line was partly clipped by VitaShell display. No touch records reported.

This is evidence for the startup probes on that repacked 0.6 executable, not Unity execution or a functioning renderer/input loop. It does not validate every ABI entrypoint, Unicode case, error path or worker behavior.

A later photograph shows VitaShell error 0x80104004 and a file named sf2-diagnostic-0.7.vpk. That VPK is not present among the locally authored artifacts, so its exact contents and hash have not been inspected. VitaSDK documents 0x80104004 as failure to parse LiveArea frame XML. Filename alone does not establish the cause in that particular package.

## Concrete source defect found

Pinned vitaGL 2bdbe895ddc6a7e410a9ea55a8f53d8173c61ad8 returns res_fallback from its initialization path. False is normal when requested resolution is supported. Our 0.6 code wrongly used if(!vglInit(...)) return 1, terminating main before drawing after a normal init. This is a confirmed code defect consistent with the report; the updated hardware test must confirm the visible fix.

Primary sources:
- [Pinned vitaGL initialization](https://github.com/Rinnegatamante/vitaGL/blob/2bdbe895ddc6a7e410a9ea55a8f53d8173c61ad8/source/vgl.c)
- [VitaSDK LiveArea errors](https://docs.vitasdk.org/group__SceLiveAreaUtilUser.html)

## 0.7-gfxtrace1 changes and acceptance

- Do not interpret init return as success/failure. Log resolution_fallback instead. Use explicit 960x544, no MSAA, 1 MiB circular pools, two display buffers and a 32 MiB diagnostic newlib heap.
- Every trace line opens/appends/flushes/closes the log, with process-time microseconds. Log before/after initialization, input, draw and swap for the first three frames; periodic heartbeat every 600 frames thereafter.
- Log kernel user/CDRAM/phycont free bytes and return code; after init also log raw free/total bytes of vitaGL pool IDs 0..4.
- Render and swap even when no controller sample is available; do not continue before drawing on a zero return from nonblocking input.
- Start a separate START-exit watcher before graphics initialization. It calls process exit without waiting for renderer/main logging; therefore an exit marker is not guaranteed if the watcher wins. Watcher creation/start errors are recorded. This cannot guarantee recovery from a kernel/system-wide lockup.
- Convert our three-color RGB(A) resources losslessly to indexed PNG8 in the build directory, preserve alpha in tRNS, validate ZIP CRC, basic XML resource structure, dimensions, SFO identity/version and SELF magic after packing. This host check is not the VitaShell XML parser and cannot certify device installation.

Install only the locally identified sf2-diagnostic-0.7-gfxtrace1.vpk. Expected log header: SF2 diagnostic 0.7-gfxtrace1. Expected own screen: UNITY NOT STARTED, LOG READY and CORE CHECK PASS. Test stick/d-pad/face buttons and START, then retrieve ux0:data/sf2vita-input.log. If the splash remains, the final stage marker distinguishes init, input, drawing or swap. Do not infer that the game has started.

193 Python tests and 8 CTest targets pass; updated VPK cross-build and package validation pass. Device verification subsequently completed for the diagnostic loop; see [0.7 hardware report](HARDWARE-0.7.md).
