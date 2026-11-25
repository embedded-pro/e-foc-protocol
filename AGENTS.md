# e-foc-protocol — Agent Rules

<!-- shared:begin 0.6.0 sha256:f57badde9903d1ff -->
Canonical rules for Claude, Copilot and sub-agents. Sections marked *(shared)* come from embedded-pro/embedded-agent-kit:
edit them there, not here. *(repo)* sections belong to this repository. *Exceptions* overrides a shared rule.

## Behaviour *(shared)*

- **Be as terse as possible.** No preamble, summary or narration. Report file paths and pass/fail.
- Everything you write — code, docs, agent files, PR text — uses the fewest words that stay unambiguous.
- Prose (docs, comments, agent files, commit and PR text) follows ASD-STE100 Simplified Technical English:
  one word, one meaning; active voice; one instruction per sentence; sentences ≤ 20 words (procedures) or ≤ 25 (descriptions).
- Ambiguous request: ask. Clear request: act.
- Do only what was asked. No unrelated refactors, no extra features.
- Batch reads, don't re-read, prefer targeted edits.
- Never change a shared section. Add a deviation to *Exceptions* only when the user approves it.

## Workflow *(shared)*

1. Multi-file or architectural change: update the design document first. Get approval before you write code.
2. Read *Domain* and *Build*. Read each `.github/instructions/*.instructions.md` whose `applyTo` matches a file you change.
3. Find the nearest existing pattern. Follow it.
4. Per behaviour: write a failing test, write the minimum code that passes it, refactor with all tests green.
5. Add each new file to its `CMakeLists.txt`.
6. Update the documentation for each behavioural change.
7. Run the *Build* commands until build and tests pass. Never disable, skip or weaken a test to get green.
8. Non-trivial change: run the `reviewer` agent, or ask the user to run it. Fix each CRITICAL and WARNING finding.
9. Report changed files, build pass/fail, test pass/fail.

## Memory *(shared)*

Scope: production code. Tests and host tools may use the heap.

- Forbidden: `new`/`delete`/`malloc`/`free`, `make_unique`/`make_shared`,
  `std::vector`/`string`/`deque`/`list`/`map`/`set`, recursion.
- Use: `infra::BoundedVector<T>::WithMaxSize<N>`, `infra::BoundedString::WithStorage<N>`,
  `infra::BoundedDeque`, `infra::BoundedList`/`IntrusiveList`, `std::array`, `std::optional`, `std::variant`.

## Interfaces & errors *(shared)*

- No virtual destructors: each adds a deleting destructor and thunks to flash.
  Interface = pure virtual methods + `protected: ~I() = default;` (non-virtual).
- Public `virtual ~Base() = default;` only on a base destroyed through a base pointer:
  `delete`/`std::unique_ptr<Base>`, `infra::PolymorphicVariant<Base, ...>`, `infra::OptionalForPolymorphicObjects<Base, ...>`.
  Put it on that base, not on every interface.
- Not needed for `std::variant`, `std::optional<Concrete>`, `infra::SharedPtr`: they destroy the concrete type.
- No exceptions. `std::optional`, `bool` or status enum; `really_assert` for preconditions. Never swallow an error.
- Constructor injection; no global state.

## Optimisation *(shared)*

- No `#pragma GCC optimize`, no `optimize` attribute: GCC won't inline across differing options.
  The consumer sets flags per translation unit.
- `OPTIMIZE_FOR_SPEED` only on hot functions defined in a header.
- Hot paths: no heap, no virtual dispatch, no blocking.

## Style *(shared)*

- `.clang-format` is authoritative. Allman, 4 spaces, `{}` init.
- PascalCase types/methods; camelCase members and enum values; lowercase namespaces.
  No `I` prefix; implementations end in `Impl`.
- `#pragma once` only.
- Fixed-width integers. `const`/`constexpr`-correct. Functions ≤ 30 lines (hard limit 50).
- Comments: non-obvious *why* only, one line. No `TODO`/`FIXME`, commented-out code, or restating the code.
  Fix or delete a comment the change makes stale.

## Testing *(shared)*

- `TEST_F` for fixtures, `TYPED_TEST` for types, plain `TEST()` for stateless checks.
- `StrictMock` only. `EXPECT_NEAR` with explicit tolerance. Fixture in an anonymous namespace, macros outside.
- Heap allowed in tests.

## Documentation *(shared)*

- Update docs before or with any behavioural change.
- One fact, one place: link, never restate.
- Visuals: Mermaid or ASCII.

## Desktop tools *(shared)*

Scope: host tools that people install and run, GUI or console.

- Open the log file first in `main`, before the application object.
  A GUI-subsystem build (`WIN32_EXECUTABLE`) has no stderr: the log file is its only output.
- Write the log to a per-user writable folder (`AppLocalDataLocation`) and rotate it.
  Capture the toolkit's messages (`qInstallMessageHandler`) from debug level.
- Install a crash handler after the log. It writes a `FATAL` line on a crash signal, `std::terminate`
  or an unhandled SEH exception, and a minidump on Windows. Qt tools use ui-cpp's `IssueLog` and `CrashReporter`.
- Log each start-up step: version, OS, toolkit version, arguments, restored settings, first frame.
- Provide `--safe-start`: it skips the saved state, so a bad saved state cannot block start-up.
- Give the log folder a menu item (Help ▸ Open Log Folder).
- Install test: start the tool, close it normally, then start it again with saved state.
  Print the log. On a failure, upload the log folder and the dumps.
<!-- shared:end -->

## Domain *(repo)*

- Versioned CAN wire contract of the e-foc motor drive: the FOC motor category (`0x02`) of can-lite, its message
  catalogue, its server (motor node side) and its client (host tools, motion controllers).
- It runs on the motor node and on hosts. *Memory* applies to all code except tests.
- Layout:
  - `e-foc-protocol/foc/` — units (`Units.hpp`, namespace `foc`), accepted parameter ranges (`CommandLimits.hpp`),
    range checks (`ParameterValidation.hpp`).
  - `e-foc-protocol/contract/` — message catalogue (`FocMotorMessages.hpp`) and wire contract
    (`FocMotorWireContract.hpp`: payload descriptors, parameter table, contract version). Header only.
  - `e-foc-protocol/server/` — `FocMotorCategoryServer`: decodes commands into observer callbacks, encodes answers
    and telemetry. What the node does with a command belongs to the firmware, never to this library.
  - `e-foc-protocol/client/` — `FocMotorCategoryClient` and `FocMotorCanClient` (protocol client, frame-origin
    tracker and category client on one `hal::Can`).
  - `*/test/` — unit tests. `*/test_doubles/` — observer mocks for consumers.
  - `integration_tests/` — cucumber-cpp: a client and a server on a virtual bus.
- Protocol code is in namespace `can`. Includes are rooted at the repository root:
  `#include "e-foc-protocol/contract/FocMotorMessages.hpp"`.
- CMake targets: `e_foc_protocol.foc`, `e_foc_protocol.contract`, `e_foc_protocol.server`, `e_foc_protocol.client`,
  `e_foc_protocol.{server,client}_test_doubles`.
- Dependencies: `embedded-infra-lib`, `can-lite`, `numerical-toolbox` come through `FetchContent`, each only when its
  target (`infra.util`, `can_lite.core`, `numerical.math`) does not exist yet. A consumer keeps its own copies.
  Pinned revisions live in the root `CMakeLists.txt`.
- Wire contract compatibility:
  - A change to an identifier, field layout, scale factor or parameter range is a contract change.
    Update `documentation/design/wire-contract.md` first, then the code.
  - Bump `focContractVersionMajor` (incompatible) or `focContractVersionMinor` (additive only).
  - A minor version only adds. Never reuse a retired identifier.
  - Every decode is length-exact. Reject a frame of another length; never decode it partially.
  - Test each command and response against literal bytes. Test client and server against each other
    (`contract/test/`).
- Math through the toolbox wrappers (`math::IsFinite`, `math::Round`, ...), never `std::` equivalents.
  Reuse numerical-toolbox (`numerical/...`); do not duplicate it.
- Unit types (`foc::Ampere`, `foc::Radians`, ...) wherever one exists.
- Only templates have method bodies in headers.
- Tests: `<component>/test/Test{Name}.cpp`, registered with `emil_build_for` / `emil_add_test`. No hardware:
  use can-lite's `CanMock` and the mocks in `test_doubles/`.
- Integration steps check values inside the mock action (`EXPECT_*` there), not in `EXPECT_CALL` matchers:
  the cucumber runner cannot report a gmock "unexpected call" failure.
- Docs: `documentation/design/wire-contract.md` is the specification. `TestWireContractDocumentation` checks it
  against the code. Requirements: `documentation/requirements/*.yaml`. New document: start from
  `documentation/templates/`.

## Build *(repo)*

```bash
cmake --preset host && cmake --build --preset host-Debug
ctest --preset host
cmake --preset bootstrap
cmake --preset cortex-m4 && cmake --build --preset cortex-m4-RelWithDebInfo
```

- Run `bootstrap` once: it fetches the toolchain files for `windows`, `cortex-m4` and `cortex-m33`.
- Presets: `host`, `host-single-Debug`, `coverage`, `bootstrap`, `windows`, `cortex-m4`, `cortex-m33`.

## Exceptions *(repo)*

None.
