add_executable(e_foc_protocol.contract_test)
emil_build_for(e_foc_protocol.contract_test HOST All PREREQUISITE_BOOL E_FOC_PROTOCOL_BUILD_TESTS)
emil_add_test(e_foc_protocol.contract_test)

target_link_libraries(e_foc_protocol.contract_test PUBLIC
    gmock_main
    e_foc_protocol.contract
    e_foc_protocol.server
    e_foc_protocol.client
    can_lite.test_util
)

target_sources(e_foc_protocol.contract_test PRIVATE
    TestFocMotorWireContract.cpp
    TestWireContractDocumentation.cpp
)

target_compile_definitions(e_foc_protocol.contract_test PRIVATE E_FOC_PROTOCOL_SOURCE_DIR="${E_FOC_PROTOCOL_SOURCE_DIR}")
