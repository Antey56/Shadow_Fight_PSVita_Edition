# First Android text logging backend

Implemented five functions in `vita/src/android_log_adapter.c`:
`__android_log_write`, `__android_log_buf_write`, `__android_log_print`,
`__android_log_buf_print`, `__android_log_vprint`.

The official NDK signatures/return contract are documented at
https://developer.android.com/ndk/reference/group/logging . The port implements
a bounded local text backend, not Android logd. A successfully delivered record
returns 1; filtering returns Android -EPERM (-1). Invalid inputs/unsupported
buffers return -EINVAL (-22); sink failure returns -EIO (-5). These values are
explicit Android numbers, not assumptions about host errno constants.

## Behavior and boundaries

- MAIN/RADIO/SYSTEM/CRASH text buffer IDs 0/1/3/4 are accepted. Binary event,
  security/kernel buffers, system-property filters and newer logging APIs are
  not implemented. Priorities 2–7 are supported; startup minimum 2–8 (8 silent).
- Text is capped at 1023 bytes and tag at 95 bytes. Truncation is reported to
  the sink. This is a deliberate bounded port policy, not a claim to reproduce
  every logd message-size limit.
- Unformatted write functions preserve literal percent signs. Print functions
  use bounded vsnprintf; vprint uses va_copy and does not consume caller state.
- The sink receives a synchronous record backed by per-call stack buffers.
  Retained data must be copied. The sink must be thread-safe and non-recursive.
- Configure before starting logging threads, or stop all logging threads first.
  Configuration mutation is not concurrently synchronized. No shared formatting
  buffer is used during logging.
- Null tag/text map to empty strings; null format is rejected. This defensive
  policy is not evidence that every invalid Android call is supported.
- Fatal assert/abort APIs are **not** replaced with nonfatal logging. No DRM,
  purchase, entitlement or license behavior is changed.

## Target integration

The declarations use `pcs("aapcs")` on ARM GCC/Clang to expose the base AAPCS
boundary. Vita target builds can populate five real ARM32 function addresses
with `sf_log_target_catalog` and feed them to `ArmSymbolScope`. Host builds
explicitly refuse this catalog, preventing truncation of host pointers into
purported target addresses.

Diagnostic startup configures the sink to write to its existing input log,
constructs the real target catalog and emits `adapter check i=7 d=1.25` through
the adapter. The CMake diagnostic version is now 0.3. It still does not load
or execute Unity or any Android game library. The old artifacts/ VPK remains
the historical 0.2 build; current 0.3 is generated in build-vita/ locally.

## Validation

- 5 CTest targets PASS, including the logging adapter tests: literal text,
  integer/double formatting, forwarded va_list with caller state preserved,
  buffers, threshold filtering, text/tag truncation, null inputs, invalid
  priority/buffer, sink failure and refusal of host target addresses.
- 61 Python/integration tests remain PASS.
- Vita cross-compilation and VPK packaging complete successfully.
- Four of the five names are imported by the provided APK; per-module users
  are listed in reports/special-1.0.12/log-adapter-status.json.

These are candidate runtime adapters. **No on-device Android-to-wrapper call
has been tested**, including Android/newlib va_list interoperability and
variadic floating-point handling. The runtime gate therefore remains blocked;
its previous 451 missing unique names must not simply be decremented by four.

Next: device ABI validation and additional audited bionic/runtime adapters,
with consistent thread-local state and calling conventions.
