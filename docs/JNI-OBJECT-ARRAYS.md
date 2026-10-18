# Restricted JNI object arrays

Implemented JNI 1.6 slots: NewObjectArray (172), GetObjectArrayElement (173), SetObjectArrayElement (174). GetArrayLength (171) accepts both primitive and these reference arrays. Layout offsets are compile-time checked. The table now has 74 implemented entries.

Elements retain owned payloads, independently of public local/global handle slots. Deleting the original reference does not invalidate a stored element. A read allocates a fresh local reference; it may fail with the existing synthetic capacity error if the 64-slot handle table is full. An array shared through a global reference retains its children across local cleanup and owner-thread detach. Replacing an element retains the replacement before releasing the old payload; failed type/bounds checks leave the element unchanged. Destruction releases all element payloads and accounting.

Supported component classes are the harness's known class tokens: String, Context, AssetManager, Class, ClassLoader, ByteBuffer/DirectByteBuffer, known Unity class tokens, primitive-array class tokens, and newly recognized java/lang/Object. Assignment uses exact known classes, Object as the common reference supertype, and DirectByteBuffer to ByteBuffer. This is not a general Java hierarchy or Android Context subclass implementation. Custom Unity instance objects are not constructed by this harness. Synthetic throwable values are not accepted as elements.

NULL initialization/elements are supported. Bounds failures and incompatible assignments set existing synthetic error codes, not real Java exception classes. Negative lengths are rejected. Limits: 4096 elements per array, 8 MiB aggregate reference-array storage per environment, separate from the primitive-array budget. Retained strings additionally share an 8 MiB payload budget per environment (including NUL terminators); this budget is returned when the final owner releases a string. It prevents arrays from bypassing the original public-handle bound on string allocations. Storage accounting includes internal element records and is host-ABI dependent. Direct-buffer payload ownership remains borrowed; array retention never extends the native allocation's lifetime.

Nested object arrays (including self-reference) are explicitly rejected, even in Object arrays. This avoids cycles in the current reference-counted model; general graphs require an object heap/collector. GetObjectClass on a reference array, array class descriptors through FindClass, array covariance, multi-dimensional arrays and array reflection are not implemented. These restrictions must be checked against an actual Unity call path before enabling execution. The slot allowlist only describes the bounded implementation, not full JNI compatibility.

## Validation

20 new Python integration tests cover retention after local deletion, independent returned locals, replacement/null, global sharing and local cleanup, bounds, stale handles, wrong classes, failed transactional assignment, primitive-array and direct-buffer elements, nested rejection, handle capacity, storage budget failure/reuse and repeated cleanup. Existing typed C table test exercises construction/read/replacement as well. Full suite: 215 Python tests and 8 CTest targets pass. Vita cross-build is checked separately. No Android code is executed by these tests.

Diagnostic source 0.7-objarrays2 adds OBJECT_ARRAY_PROBE for a String array and retained element. This is a new source identity; hardware results for 0.7-gfxtrace1 do not certify it. The terminal FatalError path is deliberately not invoked by startup probes. No claim of Unity startup, engine rendering or gameplay is made.

Reference: [Oracle JNI array operations](https://docs.oracle.com/en/java/javase/24/docs/specs/jni/functions.html#array-operations).


Hardware update 2026-10-18: 0.7-objarrays2 recorded OBJECT_ARRAY_PROBE PASS, successful core probes/draw/swap, touch IDs 0..4 and normal START exit after 211 frames. See [hardware report](../reports/HARDWARE-0.7-OBJECT-ARRAYS.md). This supersedes the pending-device wording above for this selected probe only; Unity was not started.
