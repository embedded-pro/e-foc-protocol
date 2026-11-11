#include "integration_tests/support/ProtocolFixture.hpp"

namespace integration
{
    ProtocolFixture::Connection::Connection(VirtualCan& node, VirtualCan& client)
    {
        node.ConnectTo(client);
    }

    ProtocolFixture::ProtocolFixture(uint16_t nodeId)
        : connection{ nodeCan, clientCan }
        , config{ nodeId }
        , protocolServer{ nodeCan, config }
        , server{ protocolServer.Transport() }
        , serverObserver{ server }
        , client{ clientCan }
        , clientObserver{ client.CategoryClient() }
    {
        protocolServer.RegisterCategory(server);
    }

    ProtocolFixture::~ProtocolFixture()
    {
        protocolServer.UnregisterCategory(server);
    }
}
