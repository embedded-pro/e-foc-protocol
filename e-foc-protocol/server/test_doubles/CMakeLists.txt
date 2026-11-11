add_library(e_foc_protocol.server_test_doubles INTERFACE)

target_link_libraries(e_foc_protocol.server_test_doubles INTERFACE
    e_foc_protocol.server
    gmock
)

target_sources(e_foc_protocol.server_test_doubles INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/FocMotorCategoryServerObserverMock.hpp>"
)
