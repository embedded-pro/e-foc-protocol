# e-foc-protocol — Agent Rules (canonical)

Single source of truth for **Claude, Copilot and sub-agents**. `CLAUDE.md` and
`.github/copilot-instructions.md` point here. Sub-agent definitions: `.claude/agents/` (Claude) and
`.github/agents/` (Copilot). `.github/instructions/*.instructions.md` add only file-type specifics.

Versioned CAN wire contract of the e-foc motor drive: the FOC motor category (`0x02`) of can-lite, its
message catalogue, its server (motor node side) and its client (host tools, motion controllers). It runs on
the motor node's MCU and on hosts alike, so it is held to embedded rules everywhere.

## Architecture

- `e-foc-protocol/foc/` — units (`Units.hpp`, namespace `foc`), the accepted parameter ranges
  (`CommandLimits.hpp`) and the range checks (`ParameterValidation.hpp`).
- `e-foc-protocol/contract/` — message catalogue (`FocMotorMessages.hpp`) and wire contract
  (`FocMotorWireContract.hpp`: payload descriptors, parameter table, contract version). Header only.
- `e-foc-protocol/server/` — `FocMotorCategoryServer`: decodes commands into observer callbacks and encodes
  answers and telemetry. What the node does with a command is the firmware's, never this library's.
- `e-foc-protocol/client/` — `FocMotorCategoryClient` and `FocMotorCanClient` (protocol client, frame-origin
  tracker and category client on one `hal::Can`).
- `*/test/` — unit tests; `*/test_doubles/` — observer mocks for consumers.
- `integration_tests/` — cucumber-cpp: a client and a server on a virtual bus.
- `infra/` — submodules `embedded-infra-lib`, `can-lite`, `numerical-toolbox`, used only when this repository
  is built standalone. A consumer that adds this repository with `add_subdirectory` provides those targets.

CMake targets: `e_foc_protocol.foc`, `e_foc_protocol.contract`, `e_foc_protocol.server`,
`e_foc_protocol.client`, `e_foc_protocol.{server,client}_test_doubles`. Includes are rooted at the repository
root: `#include "e-foc-protocol/contract/FocMotorMessages.hpp"`. Protocol code is in namespace `can`.

## Wire contract — compatibility

- Every change to an identifier, a field layout, a scale factor or a parameter range is a contract change:
  update `documentation/design/wire-contract.md` first, then the code, and bump `focContractVersionMajor`
  (incompatible) or `focContractVersionMinor` (additive only).
- A minor version only adds. Retired identifiers are never reused.
- Every decode is length-exact; a frame of another length is rejected, never partially decoded.
- Each command and response is tested against literal bytes, and client and server are tested against each
  other (`contract/test/`).

## Memory — no heap

Forbidden everywhere except tests: `new`/`delete`/`malloc`/`free`, `make_unique`/`make_shared`,
`std::vector`/`string`/`deque`/`list`/`map`/`set`, recursion. Use `infra::BoundedVector<T>::WithMaxSize<N>`,
`infra::BoundedString::WithStorage<N>`, `std::array<T,N>`, `std::optional<T>`.

## Embedded

- No `#pragma GCC optimize` and no `optimize` attribute: GCC does not inline a callee whose options differ
  from its caller's. The consumer sets optimisation per translation unit.
- `OPTIMIZE_FOR_SPEED` (`numerical/math/CompilerOptimizations.hpp`) only on hot functions defined in a header.
- Math through the toolbox wrappers (`math::IsFinite`, `math::Round`, ...), never `std::` equivalents.
- Unit types (`foc::Ampere`, `foc::Radians`, ...) wherever one exists.

## Interfaces & errors

- Interfaces (observers) are pure virtual with a `protected:` non-virtual `~I() = default`. Never a virtual
  destructor.
- No exceptions. `std::optional<T>`, `bool` or status enums. `really_assert` for preconditions.

## Style

- Allman braces, 4-space indent, `.clang-format` authoritative. `{}` init. PascalCase types and methods,
  camelCase members, lowercase namespaces. Fixed-width integers. Functions ≤ ~30 lines.
- **No comments** except a short non-obvious *why*. No `TODO`/`FIXME`, no commented-out code.

## Testing

- TDD. GoogleTest: `TEST_F` for fixture tests, plain `TEST()` only for simple stateless checks. Fixture in an
  anonymous namespace, macros outside.
- **`StrictMock` only** (`NiceMock`/`NaggyMock` forbidden). `EXPECT_NEAR` with an explicit tolerance.
- Test files: `<component>/test/Test{Name}.cpp`, registered with `emil_build_for` / `emil_add_test`.
- Integration steps check values inside the mock action (`EXPECT_*` there), not in `EXPECT_CALL` matchers:
  the cucumber runner cannot report a gmock "unexpected call" failure.

## Documentation

Design-first: `documentation/design/wire-contract.md` is the specification and is checked against the code by
`TestWireContractDocumentation`. Requirements: `documentation/requirements/*.yaml`. Templates:
`documentation/templates/`. Visuals: Mermaid or ASCII only.

## Build

```bash
git submodule update --init
cmake --preset host && cmake --build --preset host-Debug
ctest --preset host
cmake --preset cortex-m4 && cmake --build --preset cortex-m4-RelWithDebInfo
```

Presets: `host`, `host-single-Debug`, `coverage`, `windows`, `cortex-m4`, `cortex-m33`.

## Assistant behavior

Be terse: minimal prose, report file paths and pass/fail.
