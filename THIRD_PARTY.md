# Third-party components and source access

Project-authored sources are distributed under GPL-3.0-or-later; see LICENSE.
The diagnostic executable links third-party components under their own terms.
This repository contains no NEKKI/Shadow Fight game executable or game assets.

The following unmodified upstream components are provided by the official
VitaSDK signed channel 2026.08, sequence 2026101770. Exact build recipes are
saved in docs/dependencies. Sources can be downloaded free of charge from the
immutable links below, alongside the diagnostic VPK download in this repository.

| Component | Installed package | Corresponding source |
|---|---|---|
| vitaGL | 0.0.0.r1488.g2bdbe89-1 | https://github.com/Rinnegatamante/vitaGL/archive/2bdbe895ddc6a7e410a9ea55a8f53d8173c61ad8.tar.gz |
| vitaShaRK | 1.7-1 | https://github.com/Rinnegatamante/vitaShaRK/archive/refs/tags/v.1.7.tar.gz |
| math-neon | 0.0.0.r11.g0faab81-1 | https://github.com/Rinnegatamante/math-neon/archive/0faab814782c071ff4015527f1ca955ab1ccc470.tar.gz |
| SceShaccCgExt | 1.0.1-2 | https://github.com/bythos14/SceShaccCgExt/archive/102192366c25fe7b8fec073d665494b6172605c3.tar.gz |
| taiHEN import interface | 0.11-1 | https://github.com/yifanlu/taiHEN/tree/v0.11 |

vitaGL: copyright 2017–2020 Rinnegatamante; LGPLv3 or later, per upstream header.
vitaShaRK: LGPLv3, original notices retained in its upstream source.
math-neon: MIT, copyright 2015 Lachlan Tychsen-Smith; full notice in licenses/.
SceShaccCgExt: GPLv3, source and full license text linked/saved here.
The taiHEN kernel plugin is not included in the VPK; the program uses import stubs.

The linked C/C++ runtime and platform stubs are supplied by VitaSDK. SDK source
revisions appear in reports/vitasdk-version-info.txt; upstream source projects:
https://github.com/vitasdk/newlib , https://github.com/vitasdk/vita-headers ,
https://github.com/vitasdk/vita-toolchain . The compiler runtime carries its
upstream runtime-library exceptions; this project does not replace those terms.

No proprietary shader compiler module is distributed. vitaGL requires the user
to have libshacccg.suprx available on their own device. LiveArea images in this
repository are original generated diagnostic placeholders, not game artwork.

Rebuilding/relinking: all project sources, CMake files and dependency recipes
are supplied. Set VITASDK and run tools/build_vita.ps1 on Windows, or the
README CMake commands on Linux. All generated LiveArea art can be regenerated
with tools/make_diagnostic_art.py. Source archives are unmodified upstream
versions; package build recipes document the compilation flags.
