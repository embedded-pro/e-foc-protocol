#pragma once

#include "hal/interfaces/Can.hpp"
#include "infra/util/BoundedDeque.hpp"
#include <cstddef>

namespace integration
{
    class VirtualCan
        : public hal::Can
    {
    public:
        void SendData(Id id, const Message& data, const infra::Function<void(bool)>& onDone) override;
        void ReceiveData(const infra::Function<void(Id, const Message&)>& callback) override;
        void ConnectTo(VirtualCan& other);

    private:
        struct InFlight
        {
            Id id;
            Message data;
            infra::Function<void(bool)> onDone;
        };

        void DeliverOldest();

        static constexpr std::size_t framesInFlight{ 8 };

        VirtualCan* peer{ nullptr };
        infra::Function<void(Id, const Message&)> receiveCallback;
        infra::BoundedDeque<InFlight>::WithMaxSize<framesInFlight> inFlight;
    };
}
