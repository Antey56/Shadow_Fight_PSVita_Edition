# Ordered target symbol scope

`vita/src/arm_scope.c` provides the `ArmResolve` callback used by `arm_link_image`.
An immutable caller-owned table lists already-filtered target definitions in
the intended search order: provider name, symbol name, ARM32 value, ELF kind
(NOTYPE/OBJECT/FUNC) and binding (GLOBAL/WEAK). These are target integers, not
addresses of functions in the host Python or Windows process.

The first matching definition wins, including a weak definition. This is the
explicit policy of this component, not a claim to implement every Android
linker namespace/version/interposition rule. Constructing DT_NEEDED lookup
order, enumerating complete exports and selecting visible definitions remain
the caller's responsibility. Local/hidden/protected current-module definitions
continue to be handled by arm_link.c.

Initialization rejects duplicate provider+name pairs, unsupported kinds or
bindings, empty names and null function addresses. A concrete OBJECT/FUNC
mismatch stops resolution; a later entry is not silently substituted. NOTYPE
does not establish a C signature or float ABI. Matching ELF kinds is a useful
check, not ABI certification. Missing required definitions return an error via
the link pipeline; only an explicitly undefined weak reference may become zero.

The scope borrows table/string storage; these must remain valid and immutable.
It allocates no memory and needs no locks for concurrent read-only lookup. Init
uses quadratic duplicate checking and lookup is linear: suitable for the small
initial adapter catalog, not optimized as a full Unity export index.

## Evidence

- Host: 61 Python/integration tests PASS and 4 CTest targets PASS.
- New end-to-end fixtures use C scope callback + C ELF parser + data image +
  C relocations, covering provider resolution, first-weak selection, reversed
  explicit order, type mismatch, missing required/weak, duplicate definitions,
  null function addresses and invalid local exports.
- Vita cross-compilation including arm_scope.c completes successfully. This
  confirms compilation/linking only; no execution on a Vita was performed.

No Android adapter has been declared implemented by this change. The previously
identified 451 required external symbol names remain unresolved work. The next
integration task is populating the catalog with audited actual target adapters
and dependency definitions, with correct calling conventions and behavior.
