add_library(e_foc_protocol.integration_tests.support STATIC
    ProtocolFixture.cpp
    ProtocolFixture.hpp
    VirtualCan.cpp
    VirtualCan.hpp
)

target_include_directories(e_foc_protocol.integration_tests.support PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
)

target_link_libraries(e_foc_protocol.integration_tests.support PUBLIC
    e_foc_protocol.client_test_doubles
    e_foc_protocol.server_test_doubles
    can_lite.server
    infra.timer_test_helper
    GTest::gtest
    GTest::gmock
)
