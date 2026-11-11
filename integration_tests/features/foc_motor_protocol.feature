Feature: FOC motor protocol between a client and a motor node
  A client and a motor node share one CAN bus and talk through the FOC motor
  category: the client commands and queries, the node answers and reports.

  Background:
    Given a motor node at node 5 on the bus
    And a client on the same bus

  @REQ-WIR-002
  Scenario: The client learns the contract version of a node
    When the client queries the contract version of node 5
    Then the client shall receive the contract version of this library from node 5
    And the client shall consider node 5 compatible

  @REQ-WIR-004
  Scenario: The node activates the control mode the client selects
    When the client selects speed mode on node 5
    And the node activates speed mode
    Then the client shall receive from node 5 that speed mode is active

  @REQ-WIR-004
  Scenario: A position setpoint reaches the node unchanged
    When the client sends a position setpoint of 1.25 rad to node 5
    Then the node shall receive a position setpoint of 1.25 rad

  @REQ-WIR-004
  Scenario: Motion telemetry reaches the client
    When the node broadcasts a speed of 12.5 rad/s at position -0.75 rad
    Then the client shall receive from node 5 a speed of 12.5 rad/s at position -0.75 rad
