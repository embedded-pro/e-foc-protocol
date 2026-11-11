#include "integration_tests/support/VirtualCan.hpp"
#include "infra/event/EventDispatcher.hpp"
#include "infra/util/ReallyAssert.hpp"

namespace integration
{
    void VirtualCan::SendData(Id id, const Message& data, const infra::Function<void(bool)>& onDone)
    {
        really_assert(!inFlight.full());

        inFlight.push_back(InFlight{ id, data, onDone });
        infra::EventDispatcher::Instance().Schedule([this]()
            {
                DeliverOldest();
            });
    }

    void VirtualCan::ReceiveData(const infra::Function<void(Id, const Message&)>& callback)
    {
        receiveCallback = callback;
    }

    void VirtualCan::ConnectTo(VirtualCan& other)
    {
        peer = &other;
        other.peer = this;
    }

    void VirtualCan::DeliverOldest()
    {
        auto delivery = inFlight.front();
        inFlight.pop_front();

        if (peer != nullptr && peer->receiveCallback)
            peer->receiveCallback(delivery.id, delivery.data);

        delivery.onDone(true);
    }
}
