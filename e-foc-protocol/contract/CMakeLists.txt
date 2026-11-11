add_library(e_foc_protocol.contract INTERFACE)

target_include_directories(e_foc_protocol.contract INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../..>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(e_foc_protocol.contract INTERFACE
    e_foc_protocol.foc
    can_lite.core
    hal.interfaces
)

target_sources(e_foc_protocol.contract INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/FocMotorMessages.hpp>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/FocMotorWireContract.hpp>"
)

add_subdirectory(test)
