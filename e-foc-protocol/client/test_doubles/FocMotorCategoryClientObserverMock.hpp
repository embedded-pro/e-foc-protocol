#pragma once

#include "e-foc-protocol/client/FocMotorCategoryClient.hpp"
#include <gmock/gmock.h>

namespace can
{
    class FocMotorCategoryClientObserverMock
        : public FocMotorCategoryClientObserver
    {
    public:
        using FocMotorCategoryClientObserver::FocMotorCategoryClientObserver;

        MOCK_METHOD(void, OnSelectControlModeResponse, (uint16_t, FocMotorMode), (override));
        MOCK_METHOD(void, OnCategoryError, (uint16_t, uint8_t, FocMotorCategoryError), (override));
        MOCK_METHOD(void, OnTelemetryStatus, (uint16_t, FocMotorState, FocFaultCode), (override));
        MOCK_METHOD(void, OnTelemetryMotion, (uint16_t, foc::RadiansPerSecond, foc::Radians), (override));
        MOCK_METHOD(void, OnTelemetryCurrents, (uint16_t, uint8_t, foc::Ampere, foc::Ampere, foc::Radians), (override));
        MOCK_METHOD(void, OnContractVersionResponse, (uint16_t, uint8_t, uint8_t), (override));
        MOCK_METHOD(void, OnAlgorithmsResponse, (uint16_t, const FocAlgorithms&), (override));
        MOCK_METHOD(void, OnParameterResponse, (uint16_t, FocParameter, FocParameterValue), (override));
        MOCK_METHOD(void, OnFaultRecordResponse, (uint16_t, uint8_t, FocFaultCode, FocMotorMode, uint32_t), (override));
        MOCK_METHOD(void, OnFaultConditionsResponse, (uint16_t, uint8_t, int16_t, foc::Volts, foc::Ampere), (override));
    };
}
