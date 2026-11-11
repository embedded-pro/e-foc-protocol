# e-foc-protocol

Heap-free CAN protocol library for commanding and monitoring e-foc motor drives over
[can-lite](https://github.com/embedded-pro/can-lite).

It is the versioned wire contract of the FOC motor category (`0x02`): the message catalogue, the server the
motor node runs and the client that host tools, motion controllers and test harnesses use. The
[e-foc](https://github.com/embedded-pro/e-foc) firmware and every client build their frames from this one
definition.

## Features

- **One definition of the contract**: identifiers, payload layouts, scale factors, parameter numbers and
  their accepted ranges, with a major/minor contract version.
- **Server category**: decodes commands into typed observer callbacks; encodes answers and telemetry.
- **Client category**: typed `Send*` commands addressed by node ID, so one client commands several nodes;
  answers and telemetry are reported with the node they came from.
- **Compatibility guard**: a client refuses every command but the version query to a node whose contract
  differs from its own.
- **Length-exact decoding**: a frame of a foreign layout is rejected, never misread.
- **No heap, no exceptions**: runs on the motor node's Cortex-M and on hosts alike.
- **Test doubles**: StrictMock-ready observer mocks for consumers.

## Layout

```text
e-foc-protocol/
├── foc/          # units, accepted ranges and their checks
├── contract/     # message catalogue and wire contract (header only)
├── server/       # FocMotorCategoryServer (+ test, test_doubles)
└── client/       # FocMotorCategoryClient, FocMotorCanClient (+ test, test_doubles)
integration_tests/ # cucumber-cpp: a client and a server on a virtual bus
documentation/     # design (wire contract), architecture, requirements
```

## Using it

Add it with `FetchContent` (or as a submodule). It fetches embedded-infra-lib, can-lite and numerical-toolbox
itself only when their targets do not exist yet, so a consumer that already has them keeps its own copies:

```cmake
FetchContent_Declare(
    e_foc_protocol
    GIT_REPOSITORY https://github.com/embedded-pro/e-foc-protocol.git
    GIT_TAG        <commit>
)
FetchContent_MakeAvailable(e_foc_protocol)

target_link_libraries(my_motion_controller PUBLIC e_foc_protocol.client)
```

```cpp
#include "e-foc-protocol/client/FocMotorCanClient.hpp"
```

Targets: `e_foc_protocol.foc`, `e_foc_protocol.contract`, `e_foc_protocol.server`, `e_foc_protocol.client`,
`e_foc_protocol.server_test_doubles`, `e_foc_protocol.client_test_doubles`.

## Building and testing

```bash
git clone https://github.com/embedded-pro/e-foc-protocol.git
cmake --preset host
cmake --build --preset host-Debug
ctest --preset host
```

Compile checks for Cortex-M: `cortex-m4`, `cortex-m33` presets; run `cmake --preset bootstrap` once first, it fetches
the embedded-infra-lib toolchain files they use (as does `windows`). Coverage: `coverage` preset.

## Documentation

| Document                                                      | Description                                                  |
|---------------------------------------------------------------|--------------------------------------------------------------|
| [Wire contract](documentation/design/wire-contract.md)        | Message catalogue, parameter table, server and client design |
| [System architecture](documentation/architecture/system.md)   | Where the library sits between a motor node and its clients  |
| [Requirements](documentation/requirements/wire-contract.yaml) | Library requirements                                         |
| [Agent rules](AGENTS.md)                                      | Development rules for people and AI agents                   |

## License

See [LICENSE](LICENSE). Third-party components keep their own license terms.
