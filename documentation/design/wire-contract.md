---
title: "FOC Motor Wire Contract"
type: design
status: draft
version: 0.1.0
component: wire-contract
date: 2026-11-11
---

| Field     | Value                   |
|-----------|-------------------------|
| Title     | FOC Motor Wire Contract |
| Type      | design                  |
| Status    | draft                   |
| Version   | 0.1.0                   |
| Component | wire-contract           |
| Date      | 2026-11-11              |

> **IMPORTANT — Implementation-blind document**: This document describes *behavior, structure, and
> responsibilities* WITHOUT referencing code. **No code blocks using programming languages (C++, C,
> Python, CMake, shell, etc.) are allowed.** Use Mermaid diagrams to express behavior instead.
> Prose descriptions of algorithms are encouraged; source-level details are not.
>
> **Diagrams**: All visuals must be either a Mermaid fenced code block or ASCII art inline
> in the document. External image references (`![alt](path)`) are **not allowed**.

---

## Responsibilities

**Is responsible for:**
- Defining the FOC motor CAN category (ID `0x02`) message format — command IDs, response IDs, scale factors,
  error codes, parameter numbers and their accepted ranges — in one canonical location.
- Implementing the server category: decoding incoming CAN frames into typed observer callbacks and encoding
  typed results into response frames.
- Implementing the client category: encoding typed setpoints into CAN frames and decoding response frames into
  typed callbacks.
- Providing a client object that composes the protocol client, transport, frame-origin tracker and category
  client into one object for host tools, motion controllers and SIL tests.
- Versioning the contract, so a client refuses to command a node whose contract it does not speak.

**Is NOT responsible for:**
- The CAN protocol framing, sequence validation, or ACK dispatch — those belong to `can-lite` core.
- What a motor node does with a command: the bridge to its control-mode state machine, its liveness watchdog,
  its current streamer and its firmware-upgrade interlock belong to the e-foc firmware.
- The registry of board kinds: the contract carries a board kind as a byte; which kinds exist is the firmware's.
- Motor physics, control algorithms or hardware-specific logic.

---

## Component Details

### Part A — FOC Motor Message Catalogue

Message type IDs, the category ID, the enums and the scale factors live in `FocMotorMessages.hpp`; the field
layout of every message — order, width, signedness, scale and unit — lives beside them in
`FocMotorWireContract.hpp` as a `constexpr` descriptor table. Together these are the authoritative contract
for category `0x02`, and no other component may restate any part of them. The tables below are checked against
that table by `TestWireContractDocumentation`, so a change to one that is not made to the other fails the
build.

The category occupies slot `0x02` (the first application-reserved category ID). Commands occupy message types
`0x00–0x7F`; responses occupy `0x80–0xFF`. The category error frame uses the `can-lite` reserved type `0xFE`.

Every command carries the `can-lite` sequence byte ahead of its fields; responses and telemetry do not. A frame
whose length is not exactly the length its descriptor states is rejected by its handler and never partially
decoded; the `can-lite` protocol server answers a rejected command with `invalidPayload` and does not advance
the sequence or client-liveness bookkeeping for it — a payload of another length is a foreign layout, not a tolerable variant of this one. The
category-error frame on the `can-lite` reserved type `0xFE` carries a descriptor of its own and is held to the
same exactness, so no decode path in this category escapes it.

The descriptors are the specification and the length authority, not a codec: encoders and decoders are written
against them by hand. A descriptor edit therefore changes the accepted length and fails the byte-level vectors,
which is what surfaces the encode or decode change that has to accompany it.

**Contract version.** `focContractVersionMajor.focContractVersionMinor` identifies the contract, and
`QueryContractVersion` (`0x12`) answers it in `ContractVersionResponse` (`0x92`). A client and a server are
compatible only when both numbers are equal: the CAN Commander, the integration suite and the firmware are
built and released together, and no client keeps a path for an older or newer server. A client whose version
differs from the server's shall report the mismatch and send no further category-`0x02` commands to that
node, with one exception: it may still query the firmware-identity parameters (`0xB1`–`0xB7`, contract 2.6),
which every version since 2.6 numbers and encodes alike, so it can offer that node a firmware update
(e-foc `service-firmware-upgrade.md`). Adding a message is a minor bump; removing one, changing any field layout, or changing the meaning of
any enumerator, is a major bump. The client keeps the verdict per server node, because one client commands
several drives on a bus: an incompatible node silences commands to itself only, and a node that answers again
(after a firmware update) is judged afresh. The client remembers the eight most recent nodes, the number of
servers `can-lite` tracks per bus.

The sections that follow name the version that introduced each message and parameter. They record history:
since compatibility needs equal versions, no client branches on a minor version, and a remark below that an
older client does not ask for something describes the contract as it was, not behaviour to implement.

**Contract 3.0 — one form per command.** Version 3.0 removes every message that duplicated another one, so
each quantity has one way onto the wire. The identifiers of the removed messages are retired and never reused.

| Removed                                          | Retired IDs   | Replaced by                                                                       |
|--------------------------------------------------|---------------|-----------------------------------------------------------------------------------|
| `SetPidCurrent`, `SetPidSpeed`, `SetPidPosition` | `0x03`–`0x05` | `SetParameter` of `currentBandwidth`, `speedBandwidth`, `positionBandwidth`       |
| `SetEncoderResolution`                           | `0x09`        | `SetParameter` of `encoderResolution`                                             |
| `ConfigureTelemetryRate`                         | `0x0D`        | `SetParameter` of `telemetryRate`                                                 |
| the `int16` setpoints of 2.0                     | `0x0F`–`0x11` | the `float32` setpoints `0x1F`–`0x22` (contract 2.21)                             |
| `RequestTelemetryPrecise`                        | `0x23`        | `RequestTelemetry`, which now answers with both telemetry frames                  |
| `ElectricalParamsResponse`                       | `0x86`        | `QueryParameter` of `resistance`, `inductance`, `polePairs` after the acknowledge |

The dedicated commands accepted values the parameter table refuses — a bandwidth above the loop's maximum, an
encoder resolution above 2²⁴ — and could not select an automatic bandwidth; through `SetParameter` every
value meets the one table. The `int16` setpoints resolved 0.1 A, 1 rad/s and 0.01 rad, and the electrical
parameters response a thousandth of an ohm and a millihenry, coarser than the `float32` values that were
already on the wire; the response also went out before the calibration record that holds the values had been
accepted, so it could report a result the drive then discarded.

`TelemetryStatusResponse` (`0x89`) keeps the state and the fault code and drops the `int16` speed and position;
the measured speed and position travel only in `TelemetryMotion` (`0x8B`, named `TelemetryStatusPrecise` in
2.21), as `float32`. The two do not fit one eight-byte frame, so `RequestTelemetry` (`0x08`) is answered with
`TelemetryStatusResponse`, then `TelemetryMotion`, then the acknowledge. The emergency status broadcast on a
fault (e-foc `service-can.md` Part G) is a `TelemetryStatusResponse` and no longer carries a zero speed and position.

| ID     | Message                   | Direction | Fields (after the sequence byte for commands)     | Answer                                                             |
|--------|---------------------------|-----------|---------------------------------------------------|--------------------------------------------------------------------|
| `0x08` | `RequestTelemetry`        | command   | —                                                 | `TelemetryStatusResponse`, `TelemetryMotion`, then the acknowledge |
| `0x89` | `TelemetryStatusResponse` | telemetry | state `uint8`, fault `uint8`                      | —                                                                  |
| `0x8B` | `TelemetryMotion`         | telemetry | speed `float32` (rad/s), position `float32` (rad) | —                                                                  |

The current stream's rate is a numbered parameter beside the other configuration fields. It is stored at
once and in force within one millisecond (e-foc `service-can.md` Part J); writing it is refused `busy` while a firmware pack is
received, as every configuration write is (e-foc `service-can.md` Part K).

| ID     | Parameter       | Unit | Range                      | Automatic | Takes effect |
|--------|-----------------|------|----------------------------|-----------|--------------|
| `0x64` | `telemetryRate` | Hz   | 0 stops the stream, 1–1000 | no        | within 1 ms  |

**Contract 3.1 — position motion profile.** Version 3.1 adds four stored settings; nothing in 3.0 changes.
They are drive-wide limits like those of 2.9: they can be written in any state, are answered once stored, and a
write that cannot be stored is taken back. Position mode plans a trapezoidal path to each setpoint within them;
the profile is on only while `profileVelocity` and `profileAcceleration` are both above zero. A
`profileDeceleration` of 0 uses `profileAcceleration`. A `profileJerk` of 0 keeps the trapezoidal path; above
zero the path is an S-curve whose rate of change of acceleration stays within it. The meaning of 0x68 was
reserved until the firmware used it; its identifier, range and encoding are unchanged, so the version stays 3.1.

| ID     | Parameter             | Unit   | Range                             | Automatic | Takes effect                |
|--------|-----------------------|--------|-----------------------------------|-----------|-----------------------------|
| `0x65` | `profileVelocity`     | rad/s  | 0 – 5000, 0 profile off           | no        | at the next outer-loop tick |
| `0x66` | `profileAcceleration` | rad/s2 | 0 – 100000, 0 profile off         | no        | at the next outer-loop tick |
| `0x67` | `profileDeceleration` | rad/s2 | 0 – 100000, 0 the acceleration    | no        | at the next outer-loop tick |
| `0x68` | `profileJerk`         | rad/s3 | 0 – 10000000, 0 trapezoidal       | no        | at the next outer-loop tick |

**Contract 2.1 — algorithms and parameters.** Version 2.1 adds four commands and two responses; nothing in
2.0 changes, so a 2.0 client keeps working against a 2.1 server and a 2.1 client can tell from the minor
version whether a server understands them.

| ID     | Message              | Direction | Fields (after the sequence byte for commands)     | Answer                                     |
|--------|----------------------|-----------|---------------------------------------------------|--------------------------------------------|
| `0x13` | `SelectAlgorithm`    | command   | loop `uint8`, algorithm `uint8`                   | `AlgorithmsResponse`, then the acknowledge |
| `0x14` | `QueryAlgorithms`    | command   | —                                                 | `AlgorithmsResponse`, then the acknowledge |
| `0x15` | `SetParameter`       | command   | parameter `uint8`, value `float32`                | `ParameterResponse`, then the acknowledge  |
| `0x16` | `QueryParameter`     | command   | parameter `uint8`                                 | `ParameterResponse`, then the acknowledge  |
| `0x93` | `AlgorithmsResponse` | response  | mode, current, speed, position algorithm `uint8`  | —                                          |
| `0x96` | `ParameterResponse`  | response  | parameter `uint8`, flags `uint8`, value `float32` | —                                          |

The loop is `current = 0`, `speed = 1`, `position = 2`. The algorithm enumerations are the contract's own,
with the values the firmware's selection uses: current `pid = 0`, `decoupledPid = 1`, `deadbeat = 2`,
`slidingMode = 3`; speed `pid = 0`, `lqi = 1`, `adrc = 2`, `twoDof = 3`; position `pid = 0`, `cascadeP = 1`,
`lqr = 2`, `lqi = 3`, `twoDof = 4`. `AlgorithmsResponse` leads with the active control mode (`FocMotorMode`)
and always carries all three loops: a loop the active mode does not run reports the algorithm stored for it.
The mode is what tells a client which loops it may tune, and it is the only message that reports the mode
without changing it.

A `float32` field is the IEEE 754 single-precision bit pattern sent most significant byte first, the order of
every other multi-byte field. The parameters span from 10⁻⁹ kg·m² to 10⁶ bit/s, which no single fixed-point
scale carries, and a float keeps seven significant digits across all of them. An integer parameter is sent
as the float of that integer and is refused unless the value is whole; every integer the table allows is
exactly representable. Flag bit 0 of `ParameterResponse` is *automatic*: the value is one the firmware chose
because none was set, and setting a parameter that allows it to `0` returns it to automatic.

| ID     | Parameter                       | Unit       | Range                              | Automatic | Takes effect             |
|--------|---------------------------------|------------|------------------------------------|-----------|--------------------------|
| `0x00` | `currentBandwidth`              | rad/s      | 1 – 20000                          | yes       | immediately              |
| `0x01` | `currentSwitchingGain`          | A          | 0 – 5, below the boundary layer    | no        | immediately              |
| `0x02` | `currentBoundaryLayer`          | A          | 0.01 – 5, above the switching gain | no        | immediately              |
| `0x03` | `currentTwoStepDeadbeat`        | -          | 0 or 1                             | no        | immediately              |
| `0x10` | `speedBandwidth`                | rad/s      | 1 – 2000                           | yes       | immediately              |
| `0x11` | `speedErrorWeight`              | -          | 0.001 – 1000                       | no        | immediately              |
| `0x12` | `speedIntegralWeight`           | -          | 0.001 – 1000                       | no        | immediately              |
| `0x13` | `speedObserverBandwidthRatio`   | -          | 1 – 20                             | no        | immediately              |
| `0x14` | `speedReferenceTimeConstant`    | s          | 0.0001 – 10                        | no        | immediately              |
| `0x20` | `positionBandwidth`             | rad/s      | 1 – 500                            | yes       | immediately              |
| `0x21` | `positionErrorWeight`           | -          | 0.001 – 1000                       | no        | immediately              |
| `0x22` | `positionSpeedErrorWeight`      | -          | 0.001 – 1000                       | no        | immediately              |
| `0x23` | `positionIntegralWeight`        | -          | 0.001 – 1000                       | no        | immediately              |
| `0x24` | `positionReferenceTimeConstant` | s          | 0.0001 – 10                        | no        | immediately              |
| `0x40` | `resistance`                    | ohm        | 0.0001 – 100                       | no        | immediately              |
| `0x41` | `inductance`                    | mH         | 0.0001 – 1000                      | no        | immediately              |
| `0x42` | `polePairs`                     | -          | 1 – 64                             | no        | after the next alignment |
| `0x43` | `fluxLinkage`                   | Wb         | 0.00001 – 10                       | yes       | immediately              |
| `0x44` | `inertia`                       | kg.m2      | 0.000000001 – 10                   | no        | immediately              |
| `0x45` | `viscousFriction`               | Nm.s/rad   | 0 – 10                             | no        | immediately              |
| `0x46` | `frictionCoulomb`               | Nm         | 0 – 100                            | no        | immediately              |
| `0x60` | `encoderResolution`             | counts/rev | 1 – 16777216                       | no        | at the next start        |
| `0x61` | `canNodeId`                     | -          | 1 – 4095                           | no        | at the next start        |
| `0x62` | `canBitrate`                    | bit/s      | 125000, 250000, 500000 or 1000000  | no        | at the next start        |

The ranges are the firmware's command limits, so the table, the server's validation and the commander's
input fields share one definition. The tuning settings (`0x00`–`0x24`) are the ones described in
e-foc `controller-selection.md` Part H and are kept in the configuration record; the motor parameters
(`0x40`–`0x46`) are fields of the calibration record; `0x60`–`0x62` are configuration fields read once at
start-up. A loop's settings can only be written while the active control mode runs that loop, and a
tuning setting or motor parameter is refused unless the drive is stopped (`Idle` or `Ready`), so no
setting changes under a running law. Start-up settings are read only at start-up and can be written at any
time; a `canNodeId` base whose sum with the board ID exceeds 4095 is replaced by 1 at the next start
(e-foc `service-can.md` Part I). Every accepted write is stored, so a client that sweeps a setting rewrites the non-volatile
record on each step; tune interactively rather than in tight automated loops.

**Answers.** A write is acknowledged only once it is in force and stored: the `ParameterResponse` that
precedes the acknowledgement carries the value now held, which for a bandwidth set to `0` is the one the
firmware chose. Rejections reuse the 2.0 vocabulary:

| Situation                                                                         | Answer                                        |
|-----------------------------------------------------------------------------------|-----------------------------------------------|
| Unknown loop, algorithm or parameter; value not finite, out of range or not whole | acknowledge `invalidPayload`                  |
| Tuning setting or motor parameter while the drive is not stopped                  | acknowledge `invalidState`                    |
| Another command or a non-volatile write still outstanding                         | category error `busy`                         |
| Motor-parameter write interrupted by an emergency stop or a fault                 | category error `abortedByFault`               |
| The loop is not part of the active control mode                                   | category error `modeMismatch`                 |
| The controller cannot be designed from the stored motor model and settings        | category error `invalidParameters` (new, `6`) |
| The value is in force but could not be stored                                     | category error `persistenceFailed`            |

An interrupted motor-parameter write cannot be taken back once the record is on its way to the
non-volatile memory, so the value may still be in force after `abortedByFault`; a client reads it back.

`invalidParameters` is added to the category error enumeration; the meaning of the existing codes is
unchanged, which keeps the addition a minor version. It answers a model-based algorithm (decoupled PID,
deadbeat, speed LQI and ADRC, position LQR and LQI) selected before the motor parameters it needs are known,
a position LQR or LQI design that does not converge for the requested settings, and a switching gain that
would reach the boundary layer. In every case the law that was running keeps running unchanged.

**Contract 2.2 — commissioning and diagnostics.** Version 2.2 adds three commands and seventeen read-only
parameters; nothing in 2.1 changes. Together they reach everything the terminal offered for commissioning
and diagnosis, so a client — the CAN Commander, a host on the bus or the integration suite — needs no serial
connection (e-foc `integration-testing.md`, "Commands and Verdicts over CAN Only").

| ID     | Message            | Direction | Fields | Answer                                                         |
|--------|--------------------|-----------|--------|----------------------------------------------------------------|
| `0x17` | `Calibrate`        | command   | —      | acknowledge when the calibration sequence ends                 |
| `0x18` | `ClearCalibration` | command   | —      | acknowledge once the stored calibration record is invalidated  |
| `0x19` | `ClearDiagnostics` | command   | —      | acknowledge once the control-loop and CAN statistics are reset |

`Calibrate` runs the sequence of e-foc `state-machine.md` — current offsets, pole pairs, resistance and inductance,
alignment, the flux linkage and, in speed and position modes, the mechanical identification and the cogging
map — and stores the record, as the terminal's `calibrate` does. Like the identifications it is answered when
it ends, so a client allows it two minutes. `ClearCalibration` invalidates the stored record, as `clear_cal`
does, and the drive returns to `Idle`. Both are refused unless the drive is stopped:

| Situation                                                 | Answer                             |
|-----------------------------------------------------------|------------------------------------|
| The drive is not stopped (`Idle` or `Ready`)              | acknowledge `invalidState`         |
| Another command or a non-volatile write still outstanding | category error `busy`              |
| A step of the calibration sequence fails                  | category error `calibrationFailed` |
| Interrupted by an emergency stop or a fault               | category error `abortedByFault`    |
| The record could not be stored or invalidated             | category error `persistenceFailed` |

The read-only parameters answer `QueryParameter` like any other and are refused by `SetParameter` with
`invalidPayload`. Counts travel as the float of the count, exact up to 2²⁴ (about fourteen minutes of
control periods at 20 kHz) and rounded beyond.

| ID     | Parameter                  | Unit     | Value                                                                                        | Automatic |
|--------|----------------------------|----------|----------------------------------------------------------------------------------------------|-----------|
| `0x80` | `currentOffsetA`           | A        | Phase A current-sensor offset in the calibration record                                      | no        |
| `0x81` | `currentOffsetB`           | A        | Phase B current-sensor offset in the calibration record                                      | no        |
| `0x82` | `currentOffsetC`           | A        | Phase C current-sensor offset in the calibration record                                      | no        |
| `0x83` | `coggingPeak`              | Nm       | Peak of the stored cogging map; `0` when the record holds none                               | no        |
| `0x90` | `estimatedResistance`      | ohm      | Online estimate; `0` while the active mode runs no estimator or it has none                  | no        |
| `0x91` | `estimatedInductance`      | mH       | Online estimate, as above                                                                    | no        |
| `0x92` | `estimatedInertia`         | kg.m2    | Online estimate, as above                                                                    | no        |
| `0x93` | `estimatedViscousFriction` | Nm.s/rad | Online estimate, as above                                                                    | no        |
| `0xA0` | `loopSamples`              | -        | Control periods measured since the statistics were last reset                                | no        |
| `0xA1` | `loopBudgetCycles`         | cycles   | Cycles one control period allows                                                             | no        |
| `0xA2` | `loopMinimumCycles`        | cycles   | Shortest control-interrupt execution                                                         | no        |
| `0xA3` | `loopAverageCycles`        | cycles   | Mean control-interrupt execution                                                             | no        |
| `0xA4` | `loopMaximumCycles`        | cycles   | Longest control-interrupt execution                                                          | no        |
| `0xA5` | `loopOverruns`             | -        | Executions longer than the budget                                                            | no        |
| `0xA6` | `loopDeadlineMisses`       | -        | Periods whose interrupt started late                                                         | no        |
| `0xA7` | `loopReentries`            | -        | Interrupts that arrived while the previous one ran                                           | no        |
| `0xB0` | `resetCause`               | -        | Cause of the last reset: `powerUp 0`, `brownOut 1`, `software 2`, `hardware 3`, `watchdog 4` | no        |

**Contract 2.3 — retired messages.** Version 2.3 removes four messages that the firmware never served:
`QueryMotorType` (`0x00`) and its `MotorTypeResponse` (`0x80`) always answered `notImplemented`, and
`IdentifyMechanical` (`0x07`) with its `MechanicalParamsResponse` (`0x87`) answered `notImplemented` because the
application wires no mechanical identification to the CAN bridge; `Calibrate` (`0x17`) runs the mechanical
identification as part of the calibration sequence. Their IDs stay reserved and are never reused. A 2.2 client
that still sends either command gets no acknowledgement and times out; every other message is unchanged.

**Contract 2.4 — fault supervision.** Version 2.4 adds six parameters and one fault code; nothing in 2.3
changes. The parameters are the drive-wide supervision settings of e-foc `fault-supervision.md` (REQ-PRO-005),
kept in the configuration record. Like a start-up setting they can be written in any state and are
answered once stored, and a write that cannot be stored is taken back; unlike one, they take effect at the
supervisor's next evaluation, within 10 ms. A position error above π is never reached, because the error is
wrapped to half a turn, so a threshold that high leaves only the demanded speed as a demand.

| ID     | Parameter                    | Unit  | Range      | Automatic | Takes effect |
|--------|------------------------------|-------|------------|-----------|--------------|
| `0x70` | `supervisionWindow`          | s     | 0.05 – 10  | no        | immediately  |
| `0x71` | `supervisionCurrentFraction` | -     | 0.01 – 1   | no        | immediately  |
| `0x72` | `supervisionDemandedSpeed`   | rad/s | 0.1 – 500  | no        | immediately  |
| `0x73` | `supervisionPositionError`   | rad   | 0.01 – 3.2 | no        | immediately  |
| `0x74` | `supervisionStillExcursion`  | rad   | 0.001 – 1  | no        | immediately  |
| `0x75` | `stallDetection`             | -     | 0 or 1     | no        | immediately  |

`FocFaultCode` gains `stall` (`6`): an outside force pushed the rotor against the drive and held it
(e-foc `fault-supervision.md` Part C). It is raised only while `stallDetection` is `1`; `sensorFault` (`5`) is
kept for an encoder that stopped reporting and for a rotor locked before it moved, and cannot be switched
off. Adding the enumerator leaves the meaning of the existing codes unchanged, which keeps it a minor
version, as `invalidParameters` was in 2.1. A 2.3 client shows the new code as an unknown fault. A client
asks a server for the six parameters only when it reports 2.4 or newer, since an older one answers
`invalidPayload` for each.

**Contract 2.5 — current stream.** Version 2.5 adds one telemetry frame and retires one; no other message
changes, so a 2.4 client keeps working against a 2.5 server.

| ID     | Message             | Direction | Fields                                                                              | Answer |
|--------|---------------------|-----------|-------------------------------------------------------------------------------------|--------|
| `0x8A` | `TelemetryCurrents` | telemetry | sequence `uint8`, id `int16` (1 mA), iq `int16` (1 mA), position `int16` (0.1 mrad) | —      |

`TelemetryCurrents` is sent unasked, once per stream window, while the stream runs (e-foc `service-can.md` Part J): the window's
mean measured d- and q-axis currents and the encoder's mechanical angle at the window's last sample, in
[−π, π) from the alignment zero. `TelemetryElectrical` (`0x88`, bus voltage, current limit, Iq and Id at
0.1 A) is retired: the firmware never sent it, and the bus voltage it carried is measured only once, at
start-up. Its ID stays reserved and is never reused. A 2.4 client simply never receives either frame.

**Contract 2.6 — firmware identity.** Version 2.6 adds seven read-only parameters; nothing in 2.5 changes. They
tell a client which firmware a node runs, on which board, and which pack waits to be applied, so it can offer the right firmware update
(e-foc `service-firmware-upgrade.md` Part H). Like the diagnostics of 2.2 they answer `QueryParameter`, are refused by
`SetParameter` with `invalidPayload`, and travel as the float of the number. A client asks for them only when
the node reports 2.6 or newer.

| ID     | Parameter              | Unit | Value                                                                                                        | Automatic |
|--------|------------------------|------|--------------------------------------------------------------------------------------------------------------|-----------|
| `0xB1` | `firmwareVersionMajor` | -    | Major number of the running firmware's release version                                                       | no        |
| `0xB2` | `firmwareVersionMinor` | -    | Minor number of the running firmware's release version                                                       | no        |
| `0xB3` | `firmwareVersionPatch` | -    | Patch number of the running firmware's release version                                                       | no        |
| `0xB4` | `boardKind`            | -    | `EK-TM4C1294XL 1`, `NUCLEO-G474RE 2`, `NUCLEO-H563ZI 3`, `emulated 5`, `virtual 6`; `0` unknown; `4` retired | no        |
| `0xB5` | `stagedVersionMajor`   | -    | Major number of the release version of the pack staged for an update; `0` when none is staged                | no        |
| `0xB6` | `stagedVersionMinor`   | -    | Minor number of the staged pack's release version; `0` when none is staged                                   | no        |
| `0xB7` | `stagedVersionPatch`   | -    | Patch number of the staged pack's release version; `0` when none is staged                                   | no        |

This firmware answers all seven. The version numbers are those of the CMake project version the build was
configured with, and the board kind is the one the platform reports (`application::BoardKind`). The staged
version is the product version of the pack the firmware-upgrade service holds staged
(e-foc `service-firmware-upgrade.md` Part D); a board that cannot be updated answers it with zero.

**Contract 2.7 — missing phase.** Version 2.7 adds one fault code; nothing in 2.6 changes.

`FocFaultCode` gains `phaseMissing` (`7`): the motor's phases do not carry the current the drive drives into
them — found by the alignment's phase continuity check (e-foc `service-alignment.md`) or, while the drive runs, by
the phase-loss monitor (e-foc `fault-supervision.md` Part E). Before 2.7 the alignment failure went out as `none`
and a cable pulled while running was not reported at all. Adding the enumerator leaves the meaning of the
existing codes unchanged, which keeps it a minor version, as `stall` was in 2.4. A client older than 2.7
shows the new code as an unknown fault.

**Contract 2.8 — every fault, and Ready.** Version 2.8 adds one state and two fault codes; nothing in 2.7 changes.
Before 2.8 the fault codes `hardwareFault` (a CAN bus-off, a failed NVM write) and `calibrationFailed` went out as
`none`, and `Ready` went out as `idle`, so a client could neither tell why the drive faulted nor tell a calibrated
drive from one that still needs a calibration.

| `FocMotorState`      | Value | Lifecycle state (e-foc `state-machine.md`)                            |
|----------------------|-------|-----------------------------------------------------------------------|
| `idle`               | 0     | `Idle`: not calibrated for the active mode                            |
| `running`            | 1     | `Enabled`                                                             |
| `fault`              | 2     | `Fault`                                                               |
| `calibrating`        | 3     | `Calibrating`                                                         |
| `partialCalibration` | 4     | `Idle` holding a non-empty but incomplete record                      |
| `ready`              | 5     | `Ready`: calibrated and stopped; Start and the setpoints are accepted |

| `FocFaultCode`      | Value | `state_machine::FaultCode` |
|---------------------|-------|----------------------------|
| `none`              | 0     | `none`                     |
| `overCurrent`       | 1     | `overcurrent`              |
| `overVoltage`       | 2     | `overvoltage`              |
| `underVoltage`      | 3     | `undervoltage`             |
| `overTemperature`   | 4     | `overtemperature`          |
| `sensorFault`       | 5     | `encoderLoss`              |
| `stall`             | 6     | `stall`                    |
| `phaseMissing`      | 7     | `phaseMissing`             |
| `hardwareFault`     | 8     | `hardwareFault`            |
| `calibrationFailed` | 9     | `calibrationFailed`        |

The mapping has no default: a new `state_machine::FaultCode` fails the build until it is given a wire code.
Adding enumerators leaves the meaning of the existing ones unchanged, which keeps it a minor version, as `stall`
was in 2.4. A client older than 2.8 shows the new state and codes as unknown, and a script that waited for `idle`
after a calibration waits for `ready` from a 2.8 server. `FaultCode::watchdogTimeout` was never raised and its
value was never persisted or sent, so it was removed instead of given a wire code.

**Contract 2.9 — limits and protection.** Version 2.9 adds stored settings and fault codes; nothing in 2.8
changes. The settings are drive-wide limits kept in the configuration record (e-foc `service-nvm.md`). Like the
supervision settings of 2.4 they can be written in any state, are answered once stored, and a write that cannot
be stored is taken back; each takes effect as its table says.

| ID     | Parameter              | Unit   | Range                  | Automatic | Takes effect                                                                 |
|--------|------------------------|--------|------------------------|-----------|------------------------------------------------------------------------------|
| `0x50` | `overspeedLimit`       | rad/s  | 1 – 5000               | no        | immediately: speed limiter, setpoint hold and monitor                        |
| `0x51` | `followingErrorWindow` | rad    | 0 – 3.2, 0 off         | no        | at the monitor's next evaluation, within 1 ms                                |
| `0x52` | `followingErrorTime`   | s      | 0.001 – 10             | no        | at the monitor's next evaluation, within 1 ms                                |
| `0x53` | `speedAcceleration`    | rad/s2 | 0 – 100000, 0 no limit | no        | at the next outer-loop period (e-foc `foc-speed.md`, Speed Ramp)             |
| `0x54` | `speedDeceleration`    | rad/s2 | 0 – 100000, 0 no limit | no        | at the next outer-loop period (e-foc `foc-speed.md`, Speed Ramp)             |
| `0x55` | `commandTimeout`       | s      | 0 – 60, 0 off          | no        | at the next 1 ms evaluation (e-foc `service-can.md` Part F, Command timeout) |
| `0x56` | `continuousCurrent`    | A      | 0 – 100, 0 off         | no        | at the overload model's next evaluation (e-foc `fault-supervision.md` I)     |
| `0x57` | `peakCurrent`          | A      | 0 – 100, 0 board limit | no        | at the overload model's next evaluation (e-foc `fault-supervision.md` I)     |
| `0x58` | `peakCurrentTime`      | s      | 0.1 – 60               | no        | at the overload model's next evaluation (e-foc `fault-supervision.md` I)     |

| `FocFaultCode`   | Value | `state_machine::FaultCode` | Raised by                                                |
|------------------|-------|----------------------------|----------------------------------------------------------|
| `overSpeed`      | 10    | `overspeed`                | Overspeed monitor (e-foc `fault-supervision.md` G)       |
| `followingError` | 11    | `followingError`           | Following-error monitor (e-foc `fault-supervision.md` H) |
| `commandTimeout` | 12    | `commandTimeout`           | Command timeout (e-foc `service-can.md` Part F)          |

Adding enumerators leaves the meaning of the existing ones unchanged, which keeps it a minor version, as `stall`
was in 2.4. A client older than 2.9 shows the new codes as unknown faults, and asks for the new parameters only
when the server reports 2.9 or newer, since an older one answers `invalidPayload` for each.

**Contract 2.10 — target reached.** Version 2.10 adds two stored settings and one read-only parameter;
nothing in 2.9 changes. The settings are drive-wide limits like those of 2.9: they can be written in any state,
are answered once stored, and a write that cannot be stored is taken back.

| ID     | Parameter            | Unit | Range          | Automatic | Takes effect                                          |
|--------|----------------------|------|----------------|-----------|-------------------------------------------------------|
| `0x59` | `positionWindow`     | rad  | 0 – 3.2, 0 off | no        | within 1 ms (e-foc `foc-position.md`, Target Reached) |
| `0x5A` | `positionWindowTime` | s    | 0 – 10         | no        | within 1 ms (e-foc `foc-position.md`, Target Reached) |

| ID     | Parameter       | Unit | Value                                                             | Automatic |
|--------|-----------------|------|-------------------------------------------------------------------|-----------|
| `0xC0` | `targetReached` | -    | `1` once the position error stayed within the window for its time | no        |

A host that sequences moves sends a position setpoint and queries `targetReached` until it reads `1`. The
query is answered after the setpoint, from the error against that setpoint, so it never carries the previous
move's flag. A client older than 2.10 asks for the new parameters only when the server reports 2.10 or newer,
since an older one answers `invalidPayload` for each.

**Contract 2.11 — software position limits.** Version 2.11 adds two stored settings; nothing in 2.10 changes.
They are drive-wide limits like those of 2.9 and are off while the minimum is not below the maximum
(e-foc `foc-position.md`, Software Position Limits).

| ID     | Parameter         | Unit | Range                      | Automatic | Takes effect                                     |
|--------|-------------------|------|----------------------------|-----------|--------------------------------------------------|
| `0x5B` | `positionMinimum` | rad  | −3.2 – 3.2, off when ≥ max | no        | at the next 1 kHz tick (e-foc `foc-position.md`) |
| `0x5C` | `positionMaximum` | rad  | −3.2 – 3.2, off when ≤ min | no        | at the next 1 kHz tick (e-foc `foc-position.md`) |

With the limits on, `SetPositionSetpoint` with a setpoint outside them, taken modulo 2π into [−π, π), is
acknowledged `invalidPayload` and changes nothing, as a setpoint beyond ±2π is. A client older than 2.11
asks for the new parameters only when the server reports 2.11 or newer.

**Contract 2.12 — position limit fault.** Version 2.12 adds one fault code; nothing in 2.11 changes.

| `FocFaultCode`  | Value | `state_machine::FaultCode` | Raised by                                               |
|-----------------|-------|----------------------------|---------------------------------------------------------|
| `positionLimit` | 13    | `positionLimit`            | Position-limit monitor (e-foc `fault-supervision.md` J) |

A client older than 2.12 shows the code as an unknown fault.

**Contract 2.13 — winding temperature.** Version 2.13 adds one stored setting and one read-only parameter;
nothing in 2.12 changes (e-foc `fault-supervision.md` Part K).

| ID     | Parameter                | Unit | Range     | Automatic | Takes effect                     |
|--------|--------------------------|------|-----------|-----------|----------------------------------|
| `0x5D` | `calibrationTemperature` | degC | −40 – 100 | no        | at the next read of the estimate |

| ID     | Parameter            | Unit | Value                                                                                       | Automatic |
|--------|----------------------|------|---------------------------------------------------------------------------------------------|-----------|
| `0xC1` | `windingTemperature` | degC | Winding temperature from the online resistance; not a number in torque mode or uncalibrated | no        |

A client older than 2.13 asks for the new parameters only when the server reports 2.13 or newer.

**Contract 2.14 — over-temperature derating and trip.** Version 2.14 adds two stored settings; nothing in 2.13
changes (e-foc `fault-supervision.md` Part K). The trip raises the existing `overTemperature` (4) fault code.

| ID     | Parameter           | Unit | Range    | Automatic | Takes effect                  |
|--------|---------------------|------|----------|-----------|-------------------------------|
| `0x5E` | `temperatureLimit`  | degC | 40 – 200 | no        | at the next 100 ms evaluation |
| `0x5F` | `temperatureMargin` | K    | 1 – 50   | no        | at the next 100 ms evaluation |

A client older than 2.14 asks for the new parameters only when the server reports 2.14 or newer.

**Contract 2.15 — bus-voltage limits.** Version 2.15 adds two stored settings; nothing in 2.14 changes
(e-foc `fault-supervision.md` Part L). They take `0x30` and `0x31`, a second block of limits, as `0x50`–`0x5F` is full. A
filtered bus outside them while the drive is `Enabled` raises the existing `underVoltage` (3) or `overVoltage` (2)
fault code, as a trip of the board's comparators does.

| ID     | Parameter           | Unit | Range          | Automatic | Takes effect                    |
|--------|---------------------|------|----------------|-----------|---------------------------------|
| `0x30` | `busVoltageMinimum` | V    | 0 – 100, 0 off | no        | at the next bus value, in 10 ms |
| `0x31` | `busVoltageMaximum` | V    | 0 – 100, 0 off | no        | at the next bus value, in 10 ms |

A client older than 2.15 asks for the new parameters only when the server reports 2.15 or newer.

**Contract 2.16 — fault history.** Version 2.16 adds two commands and two responses; nothing in 2.15 changes
(e-foc `service-fault-history.md`).

| ID     | Message                   | Direction | Fields                                                                                             | Answer                                                |
|--------|---------------------------|-----------|----------------------------------------------------------------------------------------------------|-------------------------------------------------------|
| `0x1B` | `QueryFaultHistory`       | command   | age (uint8, 0 the newest)                                                                          | `FaultRecordResponse`, then `FaultConditionsResponse` |
| `0x1C` | `ClearFaultHistory`       | command   | —                                                                                                  | acknowledge once the cleared history is stored        |
| `0x9B` | `FaultRecordResponse`     | response  | age (uint8), fault code (uint8, `FocFaultCode`), mode (uint8, `FocMotorMode`), uptime (uint32, ms) | —                                                     |
| `0x9C` | `FaultConditionsResponse` | response  | age (uint8), power-up (int16), bus voltage (int16, V), peak current (int16, A)                     | —                                                     |

A query for an age past the records held is answered with the fault code `none` and zeros, so a client reads ages
from 0 until it meets one. `QueryFaultHistory` is accepted in every state; `ClearFaultHistory` too, and it is
answered once the empty history is stored, or with the category error `persistenceFailed` when the write fails.

A client older than 2.16 does not ask for the history.

**Contract 2.17 — online recalibration.** Version 2.17 adds one command, one stored setting and six read-only
parameters; nothing in 2.16 changes (e-foc `online-recalibration.md`). The command takes `0x1D`; `0x1A` stays reserved for
the quick stop.

| ID     | Message       | Direction | Fields | Answer                                                                        |
|--------|---------------|-----------|--------|-------------------------------------------------------------------------------|
| `0x1D` | `Recalibrate` | command   | —      | acknowledge once the settled estimates are adopted; the counters say how many |

`Recalibrate` is refused with `invalidState` while the drive is not stopped or the mode is off, and with the
category error `modeMismatch` in torque mode, which runs no estimators. Accepted with nothing to adopt, it changes
nothing.

| ID     | Parameter           | Unit | Range                            | Automatic | Takes effect     |
|--------|---------------------|------|----------------------------------|-----------|------------------|
| `0x76` | `recalibrationMode` | -    | 0 off, 1 on command, 2 automatic | no        | at the next stop |

| ID     | Parameter                | Unit     | Value                                                            | Automatic |
|--------|--------------------------|----------|------------------------------------------------------------------|-----------|
| `0x94` | `modelResistance`        | ohm      | Resistance the current loop is designed from; `0` in torque mode | no        |
| `0x95` | `modelInductance`        | mH       | Inductance the current loop is designed from, as above           | no        |
| `0x96` | `modelInertia`           | kg.m2    | Inertia the speed loop is designed from, as above                | no        |
| `0x97` | `modelViscousFriction`   | Nm.s/rad | Viscous friction the speed loop is designed from, as above       | no        |
| `0x98` | `recalibrationsApplied`  | -        | Quantities recalibrated since power-up                           | no        |
| `0x99` | `recalibrationsRejected` | -        | Settled readings outside their plausibility band since power-up  | no        |

The model parameters equal the calibration record until a recalibration; the motor parameters (`0x40`–`0x46`) keep
reading the record. A client older than 2.17 asks for none of them.

**Contract 2.18 — stored recalibration.** Version 2.18 adds one command and one read-only parameter; nothing in
2.17 changes (e-foc `online-recalibration.md`, Part F). From 2.18 the model parameters (`0x94`–`0x97`) also show the
inductance, inertia and friction a recalibration stored, from power-up on.

| ID     | Message      | Direction | Fields | Answer                                             |
|--------|--------------|-----------|--------|----------------------------------------------------|
| `0x1E` | `ResetModel` | command   | —      | acknowledge once the empty learned model is stored |

`ResetModel` is refused with `invalidState` while the drive is not stopped. Accepted, it stores an empty learned
model and returns the model in use to the calibration record; a write that fails is answered with the category
error `persistenceFailed`. It is accepted in every control mode and whatever the recalibration mode.

| ID     | Parameter              | Unit | Value                                                   | Automatic |
|--------|------------------------|------|---------------------------------------------------------|-----------|
| `0x9A` | `recalibrationsStored` | -    | Learned-model stores since power-up, the reset included | no        |

A client older than 2.18 does not ask for it.

**Contract 2.19 — homing.** Version 2.19 adds one stored setting and one read-only parameter; nothing in 2.18
changes (e-foc `foc-position.md`, Homing and the Position Reference). The setting is a toggle among the drive-wide
limits, stored at once and in force at the next Start.

| ID     | Parameter        | Unit | Range             | Automatic | Takes effect      |
|--------|------------------|------|-------------------|-----------|-------------------|
| `0x63` | `homingRequired` | -    | 0 off, 1 required | no        | at the next Start |

| ID     | Parameter | Unit | Value                                                                          | Automatic |
|--------|-----------|------|--------------------------------------------------------------------------------|-----------|
| `0xC2` | `homed`   | -    | `1` once the position zero is the commissioned one, `0` otherwise, in any mode | no        |

With `homingRequired` set to `1`, a `Start` in position mode is refused with the acknowledgement `rejected` unless
the drive is homed or the Start restores the reference from the encoder index; speed and torque mode ignore the
setting (e-foc `state-machine.md`, Homed State and the Homing Requirement). A client reads `homed` with each status, as it
does `targetReached`, and older than 2.19 asks for neither parameter.

**Contract 2.20 — Coulomb friction.** Version 2.20 adds one writable motor parameter; nothing in 2.19 changes
(e-foc `service-cogging-compensation.md`, Part F). `frictionCoulomb` is the dry friction of the calibration record, which
Calibrate measures in speed and position mode and which the speed and position cascades feed forward to the
current reference. It reads `0` when none was measured. Its range is `0` to `100` Nm, an upper bound well above the
dry friction of any motor the drive is built for: the server refuses a negative, non-finite or larger value with
the acknowledgement `invalidPayload`. A write is refused unless the drive is stopped, is stored in the calibration
record before it is applied, and then replaces the feed-forward at once, a value of `0` removing it, so a motor
can be commissioned from a measured value or have the feed-forward switched off without a recalibration. Torque
mode stores the value and has no feed-forward to change. A client older than 2.20 does not ask for it.

**Contract 2.21 — float32 setpoints and telemetry, torque in N·m.** Version 2.21 added the `float32` setpoints
and the `float32` motion telemetry; 3.0 removed the `int16` forms they duplicated and gave the remaining ones the
plain names below. Each carries its quantity as an IEEE-754 `float32`, most significant byte first like every
multi-byte field of the category, the encoding `SetParameter` uses, and resolves far finer than 1 mA,
0.01 rad/s and a tenth of an encoder count.

| ID     | Message                        | Direction | Fields (after the sequence byte for commands) | Answer          |
|--------|--------------------------------|-----------|-----------------------------------------------|-----------------|
| `0x1F` | `SetTorqueSetpoint`            | command   | iq `float32` (A)                              | the acknowledge |
| `0x20` | `SetTorqueSetpointNewtonMeter` | command   | torque `float32` (N·m)                        | the acknowledge |
| `0x21` | `SetSpeedSetpoint`             | command   | speed `float32` (rad/s)                       | the acknowledge |
| `0x22` | `SetPositionSetpoint`          | command   | position `float32` (rad)                      | the acknowledge |

A setpoint is accepted only in the control mode of its quantity and only while the drive is `Ready` or
`Enabled`; it is bounded by the inverter's current limit, the speed and position bounds of the command limits
and the travel limits, it feeds the command-timeout supervision and the setpoint ramp, and it is refused
`modeMismatch`, `invalidPayload` or `invalidState` in that order of checks (e-foc `service-can.md` Part D). A value that is not finite
(NaN or an infinity) is a foreign layout: the frame is rejected as `invalidPayload` before it reaches the drive.

`SetTorqueSetpointNewtonMeter` demands shaft torque instead of q-axis current. The drive converts it with the
torque constant of the calibrated motor, `1.5 · polePairs · fluxLinkage` (amplitude-invariant Clarke, the factor
the plant model applies too), the pole pairs from the calibration record and the flux linkage in force (the
recorded one, or the platform's default while none is recorded). The q-axis current that results is held to the
same current limit as `SetTorqueSetpoint`. A drive whose torque constant is not positive — no
calibration record, so no pole pairs, or no usable flux linkage — refuses the command with `invalidState`: it has
no motor to convert for. The torque constant is also a motor parameter pair a client reads (`polePairs`,
`fluxLinkage`), so a client can show the conversion it expects.

`TelemetryMotion` carries the measured speed and the encoder angle (e-foc `service-can.md` Part D, `OnRequestTelemetry`). The d- and
q-axis currents of `TelemetryCurrents` already resolve 1 mA and are not repeated; the position is the encoder's
mechanical angle from the alignment zero in [−π, π), as in the current stream.

Physical-to-wire conversions use fixed-point scale factors. `int16` fields round to nearest and
saturate (`CanFrameCodec::FloatToFixed16`):

| Quantity          | Wire type | Scale factor | Example                                    |
|-------------------|-----------|--------------|--------------------------------------------|
| Streamed current  | int16     | 1000         | 0.512 A → 512 wire; saturates at ±32.767 A |
| Streamed position | int16     | 10000        | −3.1416 rad → −31416 wire                  |
| Recorded voltage  | int16     | 100          | 40.00 V → 4000 wire                        |
| Recorded current  | int16     | 100          | 1.25 A → 125 wire                          |
| Power-up count    | int16     | 1            | 12 → 12 wire                               |

Every other quantity — setpoints, measured speed and position, and every motor parameter, from a resistance of
a few hundred milliohms to an inertia around 1e-6 kg·m² — travels as `float32`.

### One definition, verified byte for byte

The identifiers, enumerations, scale factors and field layouts of category `0x02` are defined once, in
this library; the server, the client, the CAN Commander and the integration fixtures reference that definition
and restate none of it, and every decode takes its exact payload length from it. Encoding and decoding are
written by hand against the definition rather than generated from it, so the definition alone cannot catch
a field order or width that has drifted. Two kinds of test do:

- every command and every response is checked against literal expected bytes, written independently of the
  helpers that encode and decode it, so a descriptor edit that changes a length fails them;
- the client and the server are tested against each other, not only against their own test doubles, so a
  layout both sides state wrongly in the same way is still caught.

### Deviations from the can-lite reference example

can-lite (as of `ea80b3e`) ships `examples/foc_motor`, which also claims category `0x02` and
disagrees with this contract. e-foc's definitions are authoritative for this product; the example is
documentation. Because every decode is length-exact, a frame built from the example is rejected with
`invalidPayload` rather than misread — visibly, not silently.

| Message / item                | e-foc                                       | can-lite example       | Why                                                                                      |
|-------------------------------|---------------------------------------------|------------------------|------------------------------------------------------------------------------------------|
| `0x03/0x04/0x05`              | retired, never reused                       | three `int16` kp/ki/kd | each loop derives its gains from its bandwidth (REQ-CTL-006), set through `SetParameter` |
| `0x0A`                        | `Align`                                     | reserved, unused       | alignment is a first-class command here                                                  |
| `FocMotorState`               | adds `partialCalibration = 4`, `ready = 5`  | stops at `calibrating` | speed and position modes can hold a partial record                                       |
| `0x12` / `0x92`               | contract version query and response         | absent                 | the example has no versioning                                                            |
| `0x13`–`0x16`, `0x93`, `0x96` | algorithm selection and numbered parameters | absent                 | the example has one fixed law per loop and no parameter access                           |
| category error `6`            | `invalidParameters`                         | absent                 | a model-based algorithm or setting can be refused for the motor model it would run on    |
| `0x17`–`0x19`, `0x80`–`0xC2`  | commissioning commands and read-only values | absent                 | a client commissions and diagnoses the drive without a serial connection                 |
| `0x88`                        | retired, never reused                       | `TelemetryElectrical`  | the bus voltage is measured once at start-up, so a live bus-voltage field would be stale |
| `0x8A`                        | `TelemetryCurrents` stream                  | absent                 | a host rebuilds the current and position waveforms from a stream at up to 1 kHz          |

### Part B — FocMotorCategoryServer

Inherits `CanCategoryServer` from `can-lite`. Registers one `CanMessageHandler` per command type; each handler decodes the payload using `CanPayloadReader`, skips the sequence byte (required for server categories), converts wire values to physical units, and calls the corresponding observer method.

The observer interface provides callbacks for:
- `OnStart`, `OnStop`, `OnClearFault`, `OnEmergencyStop` — lifecycle commands with a `CanAckStatus` result callback.
- `OnQueryFaultHistory`, `OnClearFaultHistory` — the fault history of contract 2.16, with a record or a status callback.
- `OnSelectControlMode` — takes a `FocMotorMode` and a result callback.
- `OnSetSetpoint` — takes a small tagged value (torque as current, torque in N·m, speed or position, as `float32` in the unit the kind names) and a completion callback; the four setpoint commands share it, so the observer surface stays small.
- `OnIdentifyElectrical` — calls `CmdReserveExternalCalibration()` before starting estimation; `invalidState` if
  rejected; waits for the rotor to come to rest and measures and applies the current-sensor offsets before it
  estimates (REQ-CMS-001); on estimation success calls `CmdCompleteExternalCalibration()` and acknowledges once it
  is accepted; on estimation failure or a rejected completion calls `CmdReleaseExternalCalibration()`.
- `OnRequestTelemetry` — broadcasts the current state and fault code via `focTelemetryStatusResponseId`, then the
  measured speed and measured position via `focTelemetryMotionId`. Position is the encoder's mechanical angle from
  the alignment zero, in [−π, π) (REQ-CTL-003); speed is the active mode's `ObserveMotion().measuredSpeed` — the
  outer loop's window speed in speed and position mode, the interrupt's filtered speed in torque mode
  (REQ-COM-002).
- `OnSelectAlgorithm`, `OnQueryAlgorithms` — receive a loop and algorithm already range-checked against the
  contract's enumerations; the result callback sends `AlgorithmsResponse` and then the acknowledgement.
- `OnSetParameter`, `OnQueryParameter` — receive a parameter number and, for a write, a value the server has
  already checked against the parameter table (finite, in range, whole where the parameter is an integer); the
  result callback sends `ParameterResponse` and then the acknowledgement. A write to a read-only parameter
  never reaches the observer: the server refuses it as `invalidPayload`.
- `OnCalibrate`, `OnClearCalibration` — commands without fields, with a `CanAckStatus` result callback that
  is called when the sequence or the invalidation ends.
- `OnClearDiagnostics` — a command without fields, with a completion callback.
- `OnRecalibrate` — the recalibration command of contract 2.17, with a `CanAckStatus` result callback.
- `OnResetModel` — the reset of the stored recalibration of contract 2.18, with a status callback.

The four setpoint handlers sit in a third handler group.
The commissioning handlers — `Align`, the electrical identification, `Calibrate` and `ClearCalibration` — sit in
one handler group of the server, and the maintenance handlers — `ClearDiagnostics`, the fault history,
`Recalibrate` and `ResetModel` — in another, so the server's own surface stays small as the contract grows.

### Part C — FocMotorCategoryClient

Inherits `CanCategoryClient` from `can-lite`. Provides typed `Send*` methods that encode physical units into wire format via `CanPayloadWriter` and call `SendCommand(nodeId, messageType, payload)`.

The three commands without fields that 2.2 adds — `Calibrate`, `ClearCalibration`, `ClearDiagnostics` — share
one send method that takes the command; like every other send it is refused for a node whose contract
version differs.

The four setpoints share one send method that takes a tagged setpoint (kind and `float32` value) and refuses a
value that is not finite before it is sent. `TelemetryStatusResponse` is decoded into a state and a fault code and
`TelemetryMotion` into a speed and a position, each on its own callback; a frame whose state is not one the
contract defines is dropped, a fault code the client does not know is passed on.

Registers response handlers for the server-to-client response message types. Response handlers decode the payload and invoke the corresponding observer callbacks, each naming the node that sent the response.

The fault history of 2.16 answers each age with two responses, and the client reports each on its own callback
with the age it carries, so a caller pairs them; it keeps no state of its own. A `FaultRecordResponse` naming no
control mode is dropped; a fault code the client does not know is passed on, so a newer firmware's code reads as
unknown rather than being lost.

`can-lite` hands a category the payload of a frame but not its sender. The client therefore takes a frame-origin
source: a thin decorator between the bus and the protocol client records the node of each received frame just
before the frame is dispatched, and the category reads it while handling that frame. Per-node contract
compatibility (see Part A) and per-node response attribution both rest on it.

### Part D — FocMotorCanClient

A self-contained client object for use in host tools and SIL tests. Internally composes the frame-origin tracker,
`CanFrameTransport`, `CanProtocolClient`, and `FocMotorCategoryClient` as value members (no heap) and registers the
category with the protocol client. Constructed with a `hal::Can&` reference only; callers send through the category
client, passing the node ID with each command, so one client can command several nodes.

---

## Interfaces

### Provided

| Interface                      | Purpose                                                            | Contract                                                                       |
|--------------------------------|--------------------------------------------------------------------|--------------------------------------------------------------------------------|
| FocMotorCategoryServerObserver | Observer interface for the server category                         | Callbacks must not block or allocate; called from the event dispatcher context |
| FocMotorCategoryClientObserver | Observer interface for the client category                         | Callbacks must not block or allocate                                           |
| FocMotorCanClient              | Wires the protocol client and the category client onto one CAN bus | Callers send through `CategoryClient()`; each `Send*` returns `bool` queued    |

### Required

| Interface           | Purpose                           | Contract                                                                |
|---------------------|-----------------------------------|-------------------------------------------------------------------------|
| `CanFrameTransport` | Sends and receives raw CAN frames | Provided by `can-lite`; must be constructed before the category objects |
| `hal::Can`          | Raw CAN bus access                | Required by `CanProtocolServer` and `CanProtocolClient`                 |

---

## Data Model

| Entity                | Field     | Type / Unit    | Range                          | Notes                                                             |
|-----------------------|-----------|----------------|--------------------------------|-------------------------------------------------------------------|
| FocMotorMode          | mode      | enum uint8     | torque=0, speed=1, position=2  | Transmitted as single byte                                        |
| Setpoint              | value     | float32 wire   | finite                         | A, N·m, rad/s or rad by command; IEEE 754, MSB first              |
| TelemetryMotion       | speed     | float32 wire   | finite                         | rad/s, the active mode's measured speed                           |
| TelemetryMotion       | position  | float32 wire   | [−π, π)                        | rad, the encoder angle from the alignment zero                    |
| FocMotorCategoryError | code      | enum uint8     | 0–6                            | See FocMotorMessages.hpp                                          |
| FocControlLoop        | loop      | enum uint8     | current=0, speed=1, position=2 | Selects the loop of `SelectAlgorithm`                             |
| FocParameter          | parameter | enum uint8     | see the parameter table        | Numbers of `SetParameter`, `QueryParameter`, `ParameterResponse`  |
| Parameter value       | value     | float32 wire   | the parameter's range          | IEEE 754, most significant byte first                             |
| ParameterResponse     | flags     | bitfield uint8 | bit 0: automatic               | Other bits are zero                                               |
| FocResetCause         | cause     | enum uint8     | 0–4                            | Value of `resetCause`, sent as its float                          |
| FocBoardKind          | kind      | enum uint8     | 0–255                          | Value of `boardKind`, sent as its float; kinds are the firmware's |
| TelemetryCurrents     | sequence  | uint8          | 0–255, wraps                   | One per window the control loop closes                            |
| TelemetryCurrents     | id, iq    | int16 wire     | INT16_MIN – INT16_MAX          | Physical = wire / 1000 [A], window mean                           |
| TelemetryCurrents     | position  | int16 wire     | −31416 – 31415                 | Physical = wire / 10000 [rad], at the window's last sample        |

---

## Constraints & Limitations

| Constraint              | Value / Description                                                                          |
|-------------------------|----------------------------------------------------------------------------------------------|
| No heap                 | All objects are value members or statically allocated; `infra::Function` for callbacks       |
| One observer per server | `CanCategoryServer` uses `infra::Subject<Observer>` — only one observer may attach at a time |
| Sequence byte           | Server handlers always skip the first byte (sequence number) before reading payload fields   |
| Tracked nodes           | A client records the contract version of at most eight nodes                                 |
