# Module lifecycle controller and authored Vita constructor test

SfModuleLifecycle binds a LINKED namespace entry to its mapped image and a
copied validated lifecycle plan. Before accepting the binding it checks the
complete init AND fini lists against the mapped file-backed executable ranges,
ARM/Thumb alignment, image base/span and handle/image identity. One owner
thread must keep namespace, mapping and dispatcher context alive and immutable.

Initialization asks the namespace to check dependency readiness, dispatches
init addresses in planned order and marks READY only after every callback
returns zero. Finalization first checks references/dependency pins, dispatches
the planned fini order and marks FINISHED. Repeated and reentrant starts/stops
are rejected. The controller copies the plan so later edits to the caller's
original plan do not change calls. Dispatcher phase 0 means init, 1 means fini.

Attempted/completed counters remain separate. A callback failure stops the
remaining calls, marks FAILED/POISONED and prevents automatic retry/teardown.
There is no rollback of partially constructed objects, no automatic unmap and
no recovery from an ARM crash, throw or longjmp crossing the dispatcher.
Dependencies remain registered/pinned. Recovery must explicitly retain the
mapping or terminate the process; it must not assume partial initialization
can be safely undone. Namespace/image raw pointers cannot be revoked by this
controller, and callers must not bypass its lifecycle or mutate its state.

## Dispatcher boundary

The controller accepts an explicit invocation callback rather than casting ARM
integers to arbitrary host function pointers. Host tests record calls and
simulate return failures; these do not execute ARM code. Android constructor
argument conventions, runtime environment, thread/TLS behavior and ABI remain
uncertified. There is no general Unity constructor dispatcher enabled.

The authored Vita probe alone uses a known void(void) base-AAPCS function
signature, because its constructor and destructor come from our own fixture.
It maps through arm_exec_map_planned, registers the module, initializes through
the controller, runs its float-entry test, finalizes, removes the registry view
and releases the VM lease. On poisoned state or failed registry removal it
exits the process rather than unmapping retained/partially initialized code.

## 0.10-ctors1 device test: READY, NOT YET EXECUTED

The test fixture now starts with initial=0. Its constructor sets initial=7 and
calls an owned lifecycle-event import; the destructor reports a second event.
The two entry results must still be 11.75 and 7.50. PASS requires exactly one
init and one fini event as well as the expected floats. C/Python planners agree
on one init and one fini call in the built fixture. Core JNI, math, mutex,
graphics/input and START diagnostics are retained. Unity/game are not started.

The dedicated build-vita-ctors cache leaves SF2_LIBMAIN_PROBE_INPUT empty.
This VPK contains authored test code only, with no original APK library.
Version 00.10, file sf2-diagnostic-0.10-ctors1.vpk, SHA-256:
31ca543729d450a8f4f277b218d712f701383c74c7963d7706aba6acde042763.
It remains local; FTP at the previous address refused the connection, so no
transfer or device result is claimed. Enable VitaShell FTP and provide its
current address before transfer. Install, launch, exit via START and re-enable
FTP to retrieve ux0:data/sf2vita-input.log.

Nine new controller tests cover order, copied plans, one-shot/reentrant calls,
partial init/fini failures, dependency readiness/pins, open-handle blocking,
invalid teardown targets, stale mappings and empty plans. Current checks:
300 Python tests and 14 CTest targets pass; VitaSDK build and VPK validation
pass. Last original-libmain hardware PASS remains 0.9-services1.


## Actual Vita result: 0.10-ctors1 PASS

Constructor at 0x835001dd ran once, followed by entry 0x8350020d returning
11.75 and 7.50. Destructor at 0x835001f5 ran once. Lifecycle result and VM
cleanup both returned 0. Event rc values 1/2 identify init/fini phases, not
errors. User free memory returned to 225443840 bytes before graphics. The
three traced frames reported GL error 0; directional and all face-button
touch IDs were observed. START exited normally after 109 frames.
This supersedes the pending device-result wording above. No original game
library was included in this build and Unity was not run.

## Consolidated testing and feasibility boundary

The current VPK already groups implemented core/JNI/arrays, selected math and
mutex ABI, ELF relocation/execution/lifecycle, memory cleanup, graphics, input
and exit into one device run. The machine-readable consolidated test plan
records both coverage and unimplemented groups; no missing group is a PASS.

Upstream Android2Vita guidance explicitly excludes Unity games from its normal
candidate path. Our diagnostic successes do not prove a runnable Unity port.
Further work must evaluate the Unity/IL2CPP runtime path, memory/API contracts
and EGL/lifecycle feasibility. A fixed remaining-step count or guaranteed
playable result cannot currently be justified.
Primary guidance: https://github.com/Rinnegatamante/Android2Vita-Candidate-Ports-List/blob/main/README.md
Reusable test candidates, not yet imported/certified here:
https://github.com/devnoname120/vita-pthread
https://android.googlesource.com/platform/bionic/+/refs/heads/main/tests/pthread_test.cpp
