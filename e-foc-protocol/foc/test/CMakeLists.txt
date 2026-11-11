add_executable(e_foc_protocol.foc_test)
emil_build_for(e_foc_protocol.foc_test HOST All BOOL E_FOC_PROTOCOL_BUILD_TESTS)
emil_add_test(e_foc_protocol.foc_test)

target_link_libraries(e_foc_protocol.foc_test PUBLIC
    gmock_main
    e_foc_protocol.foc
)

target_sources(e_foc_protocol.foc_test PRIVATE
    TestParameterValidation.cpp
    TestUnits.cpp
)
