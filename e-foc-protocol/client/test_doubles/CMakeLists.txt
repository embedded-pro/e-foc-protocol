add_library(e_foc_protocol.client_test_doubles INTERFACE)

target_link_libraries(e_foc_protocol.client_test_doubles INTERFACE
    e_foc_protocol.client
    gmock
)

target_sources(e_foc_protocol.client_test_doubles INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/FocMotorCategoryClientObserverMock.hpp>"
)
