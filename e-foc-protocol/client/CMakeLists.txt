add_library(e_foc_protocol.client STATIC)

target_include_directories(e_foc_protocol.client PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../..>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(e_foc_protocol.client PUBLIC
    e_foc_protocol.contract
    can_lite.core
    can_lite.client
    infra.util
)

target_sources(e_foc_protocol.client PRIVATE
    FocMotorCanClient.cpp
    FocMotorCanClient.hpp
    FocMotorCategoryClient.cpp
    FocMotorCategoryClient.hpp
)

emil_install(e_foc_protocol.client
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(test)
add_subdirectory(test_doubles)
