#pragma once

#include "can-lite/server/CanProtocolServer.hpp"
#include "e-foc-protocol/client/FocMotorCanClient.hpp"
#include "e-foc-protocol/client/test_doubles/FocMotorCategoryClientObserverMock.hpp"
#include "e-foc-protocol/server/FocMotorCategoryServer.hpp"
#include "e-foc-protocol/server/test_doubles/FocMotorCategoryServerObserverMock.hpp"
#include "infra/timer/test_helper/ClockFixture.hpp"
#include "integration_tests/support/VirtualCan.hpp"
#include <gmock/gmock.h>

namespace integration
{
    struct ProtocolFixture
        : infra::ClockFixture
    {
        explicit ProtocolFixture(uint16_t nodeId);
        ~ProtocolFixture();

        struct Connection
        {
            Connection(VirtualCan& node, VirtualCan& client);
        };

        VirtualCan nodeCan;
        VirtualCan clientCan;
        Connection connection;
        services::CanProtocolServer::Config config;
        services::CanProtocolServer protocolServer;
        can::FocMotorCategoryServer server;
        testing::StrictMock<can::FocMotorCategoryServerObserverMock> serverObserver;
        can::FocMotorCanClient client;
        testing::StrictMock<can::FocMotorCategoryClientObserverMock> clientObserver;
    };
}
