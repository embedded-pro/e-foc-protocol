add_executable(e_foc_protocol.server_test)
emil_build_for(e_foc_protocol.server_test HOST All PREREQUISITE_BOOL E_FOC_PROTOCOL_BUILD_TESTS)
emil_add_test(e_foc_protocol.server_test)

target_link_libraries(e_foc_protocol.server_test PUBLIC
    gmock_main
    e_foc_protocol.server
    can_lite.test_util
)

target_sources(e_foc_protocol.server_test PRIVATE
    TestFocMotorCategoryServer.cpp
)
