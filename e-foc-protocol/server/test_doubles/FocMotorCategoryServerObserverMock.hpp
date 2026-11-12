#pragma once

#include "e-foc-protocol/server/FocMotorCategoryServer.hpp"
#include <gmock/gmock.h>

namespace can
{
    class FocMotorCategoryServerObserverMock
        : public FocMotorCategoryServerObserver
    {
    public:
        using FocMotorCategoryServerObserver::FocMotorCategoryServerObserver;

        MOCK_METHOD(void, OnStart, (const infra::Function<void(services::CanAckStatus)>&), (override));
        MOCK_METHOD(void, OnStop, (const infra::Function<void(services::CanAckStatus)>&), (override));
        MOCK_METHOD(void, OnClearFault, (const infra::Function<void(services::CanAckStatus)>&), (override));
        MOCK_METHOD(void, OnEmergencyStop, (const infra::Function<void(services::CanAckStatus)>&), (override));
        MOCK_METHOD(void, OnQuickStop, (const infra::Function<void(services::CanAckStatus)>&), (override));
        MOCK_METHOD(void, OnSelectControlMode, (FocMotorMode, (const infra::Function<void(FocMotorMode)>&)), (override));
        MOCK_METHOD(void, OnSetSetpoint, (const FocSetpoint&, (const infra::Function<void()>&)), (override));
        MOCK_METHOD(void, OnAlign, ((const infra::Function<void(services::CanAckStatus)>&)), (override));
        MOCK_METHOD(void, OnIdentifyElectrical, (const infra::Function<void()>&), (override));
        MOCK_METHOD(void, OnRequestTelemetry, (const infra::Function<void()>&), (override));
        MOCK_METHOD(void, OnSelectAlgorithm, (FocControlLoop, uint8_t, (const infra::Function<void(const FocAlgorithms&)>&)), (override));
        MOCK_METHOD(void, OnQueryAlgorithms, ((const infra::Function<void(const FocAlgorithms&)>&)), (override));
        MOCK_METHOD(void, OnSetParameter, (FocParameter, float, (const infra::Function<void(FocParameterValue)>&)), (override));
        MOCK_METHOD(void, OnQueryParameter, (FocParameter, (const infra::Function<void(FocParameterValue)>&)), (override));
        MOCK_METHOD(void, OnCalibrate, ((const infra::Function<void(services::CanAckStatus)>&)), (override));
        MOCK_METHOD(void, OnClearCalibration, ((const infra::Function<void(services::CanAckStatus)>&)), (override));
        MOCK_METHOD(void, OnClearDiagnostics, ((const infra::Function<void()>&)), (override));
        MOCK_METHOD(void, OnQueryFaultHistory, (uint8_t, (const infra::Function<void(const FocFaultRecord&)>&)), (override));
        MOCK_METHOD(void, OnClearFaultHistory, ((const infra::Function<void(bool)>&)), (override));
        MOCK_METHOD(void, OnRecalibrate, ((const infra::Function<void()>&)), (override));
        MOCK_METHOD(void, OnResetModel, ((const infra::Function<void(bool)>&)), (override));
    };
}
