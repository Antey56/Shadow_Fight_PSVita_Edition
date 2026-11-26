# Shadow Fight PSVita Edition � research

> **���������� 26.11.2026:** ���������� ������� ��������� [���� franciscoclou v1.0](https://github.com/franciscoclou/shadowfight2-vita/releases/tag/v1.0), �������������� 01.11.2026. �� ������� **������� ������ 1.9.38 (1000109, ARMv7) �� Marmalade**. ��� APK Special Edition 1.0.12 �� Unity ��� ���� �� ��������. ������������ ��������� � ��������� � ���������� ���� ������� ���� � ������������ �������� �������; ��� ��������� ������� �������������. [����������� �������� � ��������� ����](reports/existing-port-verification.json). �� ��������� VPK, �� ��� ���������������� ������ ���� �� ����������� ����.


**���� ���� ������ ����������� � ������� Codex (OpenAI).** ��� ����������������� ������ �������� � ��: �������� ������ � ������� VPK �� �������� ���������� �������� �����. �� ������ ������������, �������������� ����������� 0.6 ����������� �� PS Vita � ������ ��������� probes, �� ����������� ����� �� ��������. ������ ����������� ����� 0.7-gfxtrace1 ������������ ���������/����� �������, ������� ���������� � ����� �� START ����� 239 ������; [����������� � �����������](reports/HARDWARE-0.7.md).

## ������� ���������

- **�� Vita ��������� ����������� 0.7-gfxtrace1; 0.7-objarrays2 ����� ������ ��������� ���� ��������� �������� �� Vita.** �������� ���� ���� �������� �������� � �� �����������; ��������� ������ �������� � ���� �����������.
- ��� �������� ������� vitaGL, ���������� ���������, ������� ����� � ��������� ���������� self-checks. **Shadow Fight / Unity �� �����������.**
- ���� ������������: Shadow Fight 2 Special Edition **1.0.12 / 30000019**, ARMv7, Unity 2020.3.48f1.
- ����������� C-�������� ELF, PT_DYNAMIC parser, ARM REL relocation core, ����������� mapper � VM backend ��� ������������ ��������� ELF � ����� ���������. �� Vita �������� ����������� ELF: float-������ ������� 11.75 � 7.50; [���������� �����](reports/HARDWARE-0.8.md).
- 245 Python/integration test � 11 CTest targets �������� ��������. 333366 ������� ��������� ��������� APK ������� � ����������� ��������; ���������� 317859 RELATIVE ������� ��������� ��������.
- JNI: ��������� ��������� ������� � ���������� ������ � ��������� �����; [������� ����������](docs/JNI-OBJECT-ARRAYS.md). ��������� ��������� ������� ���� �����������. [���������� ���� 0.7-objarrays2](reports/HARDWARE-0.7-OBJECT-ARRAYS.md): OBJECT_ARRAY_PROBE PASS, ���� � ����������� ���� ��������, START ��������� �����������.
- ����� `libmain.so`: ������������ ����������� ���� ������� NativeLoader � ������� �������� Unity/IL2CPP. ��������� 12 ARM RTABI memory helpers; [������ ������� ������](docs/ENGINE-BOOTSTRAP.md). ������� ������ ���������� � `0.8-elfsmoke4`, ������ ���� ���������� ������������ ELF �� Vita.
- 451 ���������� ������������ ������� ������ ���� ������� ����������/��������. Android bionic/JNI adapters, Unity lifecycle � ������ ���� ��� �������.

APK, ������� .so, assets, DRM/������������ ����� � ������������� SDK � ���� ����������� �� ������. ����� ��������, �������� � ����������� ������ �������� ����������� �������� ��������������� APK. �������/������������� ���� APK �� ���������������.

## �������� VPK �� ����� Vita

����� ����������� ��� homebrew Vita � `libshacccg.suprx`, ����������� vitaGL. ����� ����������� ������ ���������� `build-vita/sf2-diagnostic.vpk` ����� ������� ���������� VPK. Title ID: `SF2AUD001`. ��� ������ ����� ����������� JNI/JavaVM/memory probe ��� ������� ������. ��������� ������� `UNITY NOT STARTED`, ������ self-check, ��������� ����������� touch-����� ��� �������� ������ �����/���������� � �������� CROSS/SQUARE/TRIANGLE/CIRCLE. START ��������� ����������.

������: `ux0:data/sf2vita-input.log`; ������ ������ �������������� ������ ���� ��������������� ������. ������� ������ �� �����. ��� ���������� ��������� ��������� ��������� ���������� ����������; ���������� �������� ������� �����������.

## ������ � ����� �� ����������

```sh
cmake -S . -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host -j2
ctest --test-dir build-host --output-on-failure
python -m unittest discover -s tests -v
```

�� Windows ������� ��������� `-G "MinGW Makefiles"` ��� Ninja. �� Linux shared libraries ����� ���������� .so. ����� C-���������� ������� ��������������� host ������; ��� �� ����� ���� skipped. GitHub Actions ��������� ��� �������� ��� APK � ��� SDK.

## ������ VPK

VitaSDK 2026.08, GCC/G++ 15.2.0, vitaGL, vitaShaRK, SceShaccCgExt, mathneon � taihen. C++ linker ����� ��� C++-������ vitaGL, ���� ��� ��� ������� �� C.

```sh
vdpm install --noconfirm vitaGL vitaShaRK taihen
cmake -S vita -B build-vita -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-vita -j2
```

�� Windows ����� �����: `./tools/build_vita.ps1 -SdkPath C:/path/to/vitasdk`. ���������: `build-vita/sf2-diagnostic.vpk`. ������������ ������ 0.2 � � SHA-256 � [����� ������](reports/VITA-BUILD.md). ������� ��������� �������� ��������������� ������ 0.8; ��������� ���� �������� ����������. ������� ��������������� VPK ������ � ������� SDK ABI; ������� ���������� Android softfp ��������� �������������� toolchain/���������. ��������������� ������ ������ ������� �������������� ABI ����.

## ������ ������������ APK

```sh
python tools/prepare_audit.py game.apk --out reports/my-audit
python tools/check_runtime_gate.py reports/my-audit/apk-report.json --out reports/my-audit/runtime-gate.json
python tools/plan_memory.py reports/my-audit/apk-report.json --budget-mib 64 --out reports/my-audit/memory-plan.json
```

Runtime gate ������ ���������� ��� 1 ���������: ������ �� ��������. ������ 64 MiB � �������� �������� �������, �� ��������� ������ Vita. � GitHub-����� APK-������ ����������� ��� �������� ����������� ������ �� ����������� �����. ������������ ELF/DEX-������, metadata/tokens, ������ ������ exports � asset paths ���������. ������ ������ �������� � ��������� ������ � ����� ���� ������������ ������������ �� ������������ APK.

## ���������

- [����������� ������](docs/DESIGN.md)
- [����� Special Edition 1.0.12](docs/SPECIAL-1.0.12-NEXT.md)
- [��������� ������](docs/MEMORY-PLAN.md)
- [���������](docs/ARM-RELOCATIONS.md)
- [C PT_DYNAMIC parser](docs/C-DYNAMIC-PARSER.md)
- [C link pipeline](docs/LINK-PIPELINE.md)
- [������� ���������� ��������](docs/SYMBOL-SCOPE.md)
- [������ Android logging backend](docs/ANDROID-LOGGING.md)
- [������ Android assets](docs/ANDROID-ASSETS.md)
- [JNI: ����� � ���������� �����������](docs/JNI-SURFACE.md)
- [������ Context � AssetManager](docs/JNI-OBJECTS.md)
- [������������ ������� JNIEnv](docs/JNI-BRIDGE.md)
- [JavaVM � �������� ������](docs/JNI-VM.md)
- [������� ��������� ����������](docs/JNI-EXCEPTIONS.md)
- [�������� ����������� Unity](docs/UNITY-BOOTSTRAP.md)
- [������ ����� ����������� ����������](docs/INTEGRATION-STATUS.md)
- [����������� Unity, ������ � reflection](docs/JNI-REGISTRATION-REFLECTION.md)
- [JNI-������� � ������ ByteBuffer](docs/JNI-ARRAYS-BUFFERS.md)
- [������ ���������� ����� � ����������� 0.7](reports/HARDWARE-0.6.md)
- [��������� � ���� ��](CONTRIBUTORS.md)
- [��������� � �������� ������������](THIRD_PARTY.md)

��� ������� ��������������� ��� ��������; ������ �������� ��������� ����������� ��������� ��������. Shadow Fight, NEKKI � Unity � ������������� ��������������� ����������������; ������ �� �������� �� ����������� ���������.


������� ��������� ����������� � [JNI_OnLoad ������������� libmain](docs/LIBMAIN-ENTRY-PROBE.md), ������ 0.9-libmain1. �� �� �������� NativeLoader.load � �� ��������� Unity. ��������� ������������ ���������� ������� ������ ���������� ���� � �������� SHA; �������� � ��� �� �����������.


Hardware milestone: original libmain JNI_OnLoad passes on Vita in 0.9-libmain2; see [result and limits](reports/HARDWARE-0.9.md). Unity and gameplay have not started.


Current local diagnostic: [0.10-ctors1](docs/MODULE-LIFECYCLE-CONTROLLER.md), authored ARM constructor/destructor test, built and validated but awaiting device transfer/run. Unity/game remain unstarted.
