add_library(e_foc_protocol.integration_tests.steps OBJECT
    FocMotorProtocolSteps.cpp
)

target_link_libraries(e_foc_protocol.integration_tests.steps PUBLIC
    e_foc_protocol.integration_tests.support
    cucumber_cpp.runner
    GTest::gtest
    GTest::gmock
)
