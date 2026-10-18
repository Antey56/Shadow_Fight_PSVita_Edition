# Vita diagnostic build — 18 October 2026

**VPK successfully built; not yet run on a physical Vita.**

- File: artifacts/sf2-diagnostic.vpk
- SHA-256: `6691fb9583bf694c0ff9e5da0ddf0ea2dfadcd1a0e7732648a288108ae4f4873`
- Size: 470628 bytes
- Title ID: SF2AUD001; version: 00.02
- Native Windows VitaSDK release 2026.08, signed channel sequence 2026101770
- Toolchain sdk-snapshot-20260825.611.1; GCC/G++ 15.2.0
- Package snapshot packages-2026.08-snapshot-20261017.59.1
- vitaGL 0.0.0.r1488.g2bdbe89-1; vitaShaRK 1.7-1; SceShaccCgExt 1.0.1-2; libmathneon 0.0.0.r11.g0faab81-1; taihen 0.11-1

Build: CMake + Ninja, Release; C++ linker plus vitaGL, vitaShaRK, SceShaccCgExt, mathneon and platform stubs. The initial missing C++/mathneon/extension link dependencies were fixed in vita/CMakeLists.txt. Build completes without compiler warnings in the captured final run.

Offline verification: VPK ZIP CRCs valid, SELF magic SCE\0, param.sfo title/version checked, icon and LiveArea assets present. This checks packaging, not on-device installation or rendering. The visible labels and input/self-check indicators describe expected behavior until device validation.

No game code/assets included. Unity/JNI are not started. This normal-ABI diagnostic build does not certify softfp Android runtime interoperability.

See THIRD_PARTY.md for source downloads and dependency licenses. Earlier reports describing an unavailable VitaSDK are historical; this report supersedes that build-environment limitation.
