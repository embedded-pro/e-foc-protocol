add_library(e_foc_protocol.server STATIC)

target_include_directories(e_foc_protocol.server PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../..>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(e_foc_protocol.server PUBLIC
    e_foc_protocol.contract
    can_lite.core
    infra.util
)

target_sources(e_foc_protocol.server PRIVATE
    FocMotorCategoryServer.cpp
    FocMotorCategoryServer.hpp
)

emil_install(e_foc_protocol.server
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(test)
add_subdirectory(test_doubles)
