#include "cucumber_cpp/Steps.hpp"
#include "e-foc-protocol/contract/FocMotorWireContract.hpp"
#include "integration_tests/support/ProtocolFixture.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using integration::ProtocolFixture;
using testing::_;

namespace
{
    constexpr float tolerance{ 1.0e-6f };
}

GIVEN(R"(a motor node at node {int} on the bus)", (std::int32_t node))
{
    context.Emplace<ProtocolFixture>(static_cast<uint16_t>(node));
}

GIVEN(R"(a client on the same bus)")
{
    context.Get<ProtocolFixture>().ExecuteAllActions();
}

WHEN(R"(the client queries the contract version of node {int})", (std::int32_t node))
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_TRUE(fixture.client.CategoryClient().SendQueryContractVersion(static_cast<uint16_t>(node)));
}

THEN(R"(the client shall receive the contract version of this library from node {int})", (std::int32_t node))
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_CALL(fixture.clientObserver, OnContractVersionResponse(_, _, _))
        .WillOnce([node](uint16_t sourceNode, uint8_t major, uint8_t minor)
            {
                EXPECT_EQ(node, sourceNode);
                EXPECT_EQ(can::focContractVersionMajor, major);
                EXPECT_EQ(can::focContractVersionMinor, minor);
            });
    fixture.ExecuteAllActions();
}

THEN(R"(the client shall consider node {int} compatible)", (std::int32_t node))
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_EQ(can::ContractCompatibility::compatible, fixture.client.CategoryClient().Compatibility(static_cast<uint16_t>(node)));
}

WHEN(R"(the client selects speed mode on node {int})", (std::int32_t node))
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_TRUE(fixture.client.CategoryClient().SendSelectControlMode(static_cast<uint16_t>(node), can::FocMotorMode::speed));
}

WHEN(R"(the node activates speed mode)")
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_CALL(fixture.serverObserver, OnSelectControlMode(_, _))
        .WillOnce([](can::FocMotorMode mode, const infra::Function<void(can::FocMotorMode)>& onActivated)
            {
                EXPECT_EQ(can::FocMotorMode::speed, mode);
                onActivated(mode);
            });
}

THEN(R"(the client shall receive from node {int} that speed mode is active)", (std::int32_t node))
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_CALL(fixture.clientObserver, OnSelectControlModeResponse(_, _))
        .WillOnce([node](uint16_t sourceNode, can::FocMotorMode activeMode)
            {
                EXPECT_EQ(node, sourceNode);
                EXPECT_EQ(can::FocMotorMode::speed, activeMode);
            });
    fixture.ExecuteAllActions();
}

WHEN(R"(the client sends a position setpoint of {float} rad to node {int})", (float position, std::int32_t node))
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_TRUE(fixture.client.CategoryClient().SendSetpoint(static_cast<uint16_t>(node), can::FocSetpoint{ can::FocSetpointKind::position, position }));
}

THEN(R"(the node shall receive a position setpoint of {float} rad)", (float position))
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_CALL(fixture.serverObserver, OnSetSetpoint(_, _))
        .WillOnce([position](const can::FocSetpoint& setpoint, const infra::Function<void()>& onDone)
            {
                EXPECT_EQ(can::FocSetpointKind::position, setpoint.kind);
                EXPECT_NEAR(position, setpoint.value, tolerance);
                onDone();
            });
    fixture.ExecuteAllActions();
}

WHEN(R"(the node broadcasts a speed of {float} rad\/s at position {float} rad)", (float speed, float position))
{
    auto& fixture = context.Get<ProtocolFixture>();
    fixture.server.BroadcastTelemetryMotion(foc::RadiansPerSecond{ speed }, foc::Radians{ position });
}

THEN(R"(the client shall receive from node {int} a speed of {float} rad\/s at position {float} rad)", (std::int32_t node, float speed, float position))
{
    auto& fixture = context.Get<ProtocolFixture>();
    EXPECT_CALL(fixture.clientObserver, OnTelemetryMotion(_, _, _))
        .WillOnce([node, speed, position](uint16_t sourceNode, foc::RadiansPerSecond measuredSpeed, foc::Radians measuredPosition)
            {
                EXPECT_EQ(node, sourceNode);
                EXPECT_NEAR(speed, measuredSpeed.Value(), tolerance);
                EXPECT_NEAR(position, measuredPosition.Value(), tolerance);
            });
    fixture.ExecuteAllActions();
}
