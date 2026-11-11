add_executable(e_foc_protocol.integration_tests)
emil_build_for(e_foc_protocol.integration_tests HOST All PREREQUISITE_BOOL E_FOC_PROTOCOL_BUILD_TESTS)

target_link_libraries(e_foc_protocol.integration_tests PRIVATE
    e_foc_protocol.integration_tests.steps
    e_foc_protocol.integration_tests.hooks
    e_foc_protocol.integration_tests.support
    cucumber_cpp.runner
)

add_test(NAME e_foc_protocol.integration_tests
    COMMAND e_foc_protocol.integration_tests -- ${CMAKE_CURRENT_SOURCE_DIR}/../features)
set_tests_properties(e_foc_protocol.integration_tests PROPERTIES LABELS integration)
