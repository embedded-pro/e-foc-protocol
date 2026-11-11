# integration_tests

BDD integration tests using [cucumber-cpp](https://github.com/philips-software/amp-cucumber-cpp-runner).
A `FocMotorCanClient` and a `FocMotorCategoryServer` behind a can-lite protocol server share a virtual
CAN bus; the scenarios check that commands, answers and telemetry cross it unchanged.

```
integration_tests/
├── features/   # Gherkin specification, tagged with the requirement each scenario verifies
├── support/    # ProtocolFixture (client, server, observers) and the virtual bus
├── hooks/      # before/after scenario hooks
├── steps/      # step definitions
└── main/       # test executable
```

Built when `E_FOC_PROTOCOL_BUILD_TESTS` is on (the `host` presets enable it) and run by CTest:

```bash
ctest --preset host -R e_foc_protocol.integration_tests
```

Frames travel through the event dispatcher, so a step that sends only queues the frame; the `Then` step sets
its expectation and then runs the dispatcher. Check values inside the mock action: the runner cannot report a
gmock "unexpected call".
