#pragma once

#include "can-lite/client/CanProtocolClient.hpp"
#include "can-lite/core/CanFrameOrigin.hpp"
#include "e-foc-protocol/client/FocMotorCategoryClient.hpp"
#include "hal/interfaces/Can.hpp"

namespace can
{
    class FocMotorCanClient
    {
    public:
        explicit FocMotorCanClient(hal::Can& can);

        services::CanProtocolClient& ProtocolClient();
        FocMotorCategoryClient& CategoryClient();
        const FocMotorCategoryClient& CategoryClient() const;
        const services::CanFrameOrigin& Origin() const;

    private:
        services::CanFrameOriginTracker originTracker;
        services::CanProtocolClient protocolClient;
        FocMotorCategoryClient categoryClient;
    };
}
