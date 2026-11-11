add_executable(e_foc_protocol.client_test)
emil_build_for(e_foc_protocol.client_test HOST All BOOL E_FOC_PROTOCOL_BUILD_TESTS)
emil_add_test(e_foc_protocol.client_test)

target_link_libraries(e_foc_protocol.client_test PUBLIC
    gmock_main
    e_foc_protocol.client
    can_lite.test_util
    infra.timer_test_helper
)

target_sources(e_foc_protocol.client_test PRIVATE
    TestFocMotorCategoryClient.cpp
    TestFocMotorCanClient.cpp
)
