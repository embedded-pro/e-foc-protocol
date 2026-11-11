add_library(e_foc_protocol.foc INTERFACE)

target_include_directories(e_foc_protocol.foc INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../..>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(e_foc_protocol.foc INTERFACE
    infra.util
    numerical.math
)

target_sources(e_foc_protocol.foc INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/CommandLimits.hpp>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/ParameterValidation.hpp>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/Units.hpp>"
)

add_subdirectory(test)
