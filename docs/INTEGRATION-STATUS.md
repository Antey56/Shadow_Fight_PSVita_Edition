# Six integration areas: current evidence and blockers

Оценка «шесть шагов» обозначала крупные направления, а не шесть коротких исправлений. Сейчас получен проверяемый исследовательский каркас и диагностическая сборка; готового запуска игры нет. [Android2Vita candidate requirements](https://github.com/Rinnegatamante/Android2Vita-Candidate-Ports-List#android-port-requirements-for-vita-so-loader) исключают libunity из обычного поддерживаемого пути. Это не доказательство абсолютной невозможности другого исследования, но готового стандартного Unity runtime данный проект не имеет.

| Area | Result | Remaining prerequisite |
|---|---|---|
| JNI registrations | 51 pointer-table candidates, 28 statically confirmed methods in three groups | Remaining groups and actual lifecycle/class handling |
| JNI runtime | 16 restricted slots; owner JavaVM; synthetic error references | RegisterNatives, ClassLoader/Class reflection, strings/arrays, worker environments |
| Android/bionic | Logging, assets, 20 memory/string candidate adapters | 451 unique required names still uncertified; pthread, TLS, FILE/stat and platform semantics |
| Executable loader | ELF/dynamic/scope/relocations and data image tested | Executable memory, caches, ARM ABI, complete imports and services before constructors |
| Graphics/audio | vitaGL diagnostic only; API names inventoried | Engine EGL surfaces, contexts, synchronization, audio and codec paths |
| Physical Vita | Diagnostic 0.4 built | Device execution and captured logs; no connected Vita available |

assess_port_readiness.py produces port-readiness.json from the original reports and bootstrap profile. It always reports execution_allowed=false: filling observed JNI slots cannot prove the completeness of a call graph or runtime. No fixture or report unlocks execution.

## Latest checkpoint

Diagnostic 0.5 and JNI-REGISTRATION-REFLECTION.md supersede the earlier JNI details in the table above. RegisterNatives metadata replay now verifies all 28 methods; 26 restricted slots include modified UTF-8 strings and known-class reflection. Java initialization, arrays, shared worker environments and the platform/executable-loader layers remain incomplete. Current checks: 161 Python tests and 8 CTest targets.

## Arrays checkpoint

Diagnostic 0.6 adds bounded primitive arrays and borrowed direct ByteBuffer metadata. JNI-ARRAYS-BUFFERS.md describes the limits. The current table has 70 non-NULL slots; object arrays, class initialization, workers and engine platform integration remain incomplete. Current checks: 183 Python tests, 8 CTest targets.

## New memory/string adapters

bionic_memory.c contains 20 explicit base-AAPCS wrappers with target-only address catalog. They cover malloc/calloc/realloc/free, byte memory operations and common narrow string operations. Nineteen names occur among this APK's unresolved required imports. They use one newlib allocator family, including strdup; never free an allocation from another runtime with these functions.

No bionic FILE, stat, pthread, wchar, locale, C++ unwinding or errno bridge is implied. calloc explicitly checks multiplication overflow; that early rejection currently does not establish Android errno behavior. Invalid pointers, overlapping memcpy or undersized destination buffers retain the underlying standard-C preconditions. No generic NULL-success stubs or arbitrary struct forwarding were added. These are host-tested candidates, not certified game runtime imports, so the 451-name certification count is unchanged.

## Platform observations

libunity alone has 372 required unresolved import records; libil2cpp has 265 and libmain 30 (counts overlap across modules). Unity directly imports EGL context/surface operations, ANativeWindow functions and pthread synchronization/thread creation. These functions need real semantics; a display-only vitaGL loop does not satisfy them.

Dynamic string inventory contains 306 gl names, 28 egl names, 179 vk names and 50 AMedia names. Such strings do not prove the backend selected by this build, GLES level, runtime invocation or a mandatory Vulkan/codec dependency. They are investigation targets, not an excuse to claim that all listed APIs must be emulated. Audio is additionally tied to Java-side FMODAudioDevice declarations and direct-buffer processing, which remain unsupported.

## Diagnostic 0.4

The same Vita executable now runs a small owned JavaVM/JNIEnv probe before graphics: Context → getAssets → manager conversion, synthetic exception capture/clear/rethrow, detach and VM cleanup. It also creates the 20-address memory catalog and exercises strdup/free. It opens no game assets and executes no Android instructions. Expected log is ux0:data/sf2vita-input.log with JNI/memory probe=PASS; no device result is claimed until observed.

Build locally using README. Binaries remain local. Original APK, game libraries, assets and executable-memory shortcuts are not published. The next actionable implementation is exact JNI registration/class semantics against the verified tables, followed by separately verified bionic/platform adapters; constructors must remain disabled while dependencies are missing.

Validation: 126 Python/integration tests, 8 CTest targets, successful Vita cross-build. Host tests include table relocation evidence, hash mismatch rejection, read/write overlap and allocator boundaries. Hardware tests, first frame, sound and gameplay are not complete.


## 2026-10-18: diagnostic hardware and terminal JNI failure path

See [hardware evidence](../reports/HARDWARE-0.7.md): 0.7-gfxtrace1 returned from drawing/swapping and recorded control events and normal START exit on Vita. This supersedes historical statements above that no diagnostic hardware test has run. Unity and engine integration remain untested.

Current source identity is 0.7-jni-fatal1, distinct from the tested executable. The JNI table now has 71 implemented slots. Slot 18, FatalError, logs a bounded message and calls abort without returning; on Vita it also appends to the diagnostic log. It is not a recoverable exception, optional service fallback or license workaround. Host child-process tests verify termination for text and NULL messages and literal format characters. The fatal path itself has not been invoked on Vita. No new VPK installation is necessary to reproduce the earlier graphics result.

195 Python tests and 8 CTest targets pass. Vita cross-build/package validation pass. Zero missing slots in the statically observed subset does not imply complete JNI semantics: worker threads, initialization, object arrays, executable loading and 451 uncertified required imports still block Unity execution.

Reference: [JNI FatalError specification](https://docs.oracle.com/en/java/javase/24/docs/specs/jni/functions.html#fatalerror).


## 2026-10-18: reference arrays

Current source 0.7-objarrays1 adds three leaf object-array slots, bringing the table to 74 implemented entries. See [ownership, limits and tests](JNI-OBJECT-ARRAYS.md). Nested arrays, initialization and workers remain incomplete. 214 Python tests and 8 CTest targets pass; the new array probe has not been tested on Vita. Earlier 0.7 hardware results remain scoped to gfxtrace1.
