cmake_minimum_required(VERSION 3.24)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(E_FOC_PROTOCOL_STANDALONE On)
endif()

option(CMAKE_COMPILE_WARNING_AS_ERROR "Enable warnings-as-error" On)
option(E_FOC_PROTOCOL_BUILD_TESTS "Enable build of the tests" Off)

if (E_FOC_PROTOCOL_BUILD_TESTS)
    set(BUILD_TESTING On)
endif()

project(e_foc_protocol LANGUAGES C CXX ASM VERSION 0.0.1) # x-release-please-version

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set(E_FOC_PROTOCOL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(GNUInstallDirs)

if (E_FOC_PROTOCOL_STANDALONE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS_DEBUG "-Og -g" CACHE STRING "C Debug flags" FORCE)
        set(CMAKE_CXX_FLAGS_DEBUG "-Og -g" CACHE STRING "C++ Debug flags" FORCE)
    endif()

    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/MarkIncludesSystem.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerWarnings.cmake")

    set_directory_properties(PROPERTY USE_FOLDERS ON)

    set(EMIL_INCLUDE_ECHO Off CACHE BOOL "" FORCE)
    set(EMIL_FETCH_ECHO_COMPILERS Off CACHE BOOL "" FORCE)
    set(EMIL_BUILD_ECHO_COMPILERS Off CACHE BOOL "" FORCE)
    set(EMIL_ENABLE_DOCKER_TOOLS Off CACHE BOOL "" FORCE)
    set(EMIL_INCLUDE_MBEDTLS Off CACHE BOOL "" FORCE)

    add_subdirectory(infra/embedded-infra-lib)

    if (E_FOC_PROTOCOL_BUILD_TESTS)
        set(CCR_FETCH_DEPS On CACHE BOOL "" FORCE)
        set(CCR_BUILD_TESTS Off CACHE BOOL "" FORCE)

        include(FetchContent)
        FetchContent_Declare(
            cucumber_cpp
            GIT_REPOSITORY https://github.com/philips-software/amp-cucumber-cpp-runner.git
            GIT_TAG        v4.1.0
        )
        FetchContent_MakeAvailable(cucumber_cpp)

        if (TARGET cucumber_cpp.library.api)
            target_compile_options(cucumber_cpp.library.api PRIVATE -Wno-deprecated-declarations)
        endif()
    endif()

    add_subdirectory(infra/can-lite)
    add_subdirectory(infra/numerical-toolbox)

    if (E_FOC_PROTOCOL_BUILD_TESTS)
        include(CTest)
        emil_enable_testing()
    endif()

    e_foc_protocol_mark_includes_system("${CMAKE_CURRENT_SOURCE_DIR}")
    e_foc_protocol_enable_project_warnings()
endif()

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME e_foc_protocol)

add_subdirectory(e-foc-protocol)

install(DIRECTORY e-foc-protocol
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
    PATTERN "test" EXCLUDE
    PATTERN "test_doubles" EXCLUDE
)

unset(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME)

if (NOT E_FOC_PROTOCOL_STANDALONE)
    return()
endif()

set(E_FOC_PROTOCOL_FORMAT_DIRECTORIES e-foc-protocol)

if (E_FOC_PROTOCOL_BUILD_TESTS)
    add_subdirectory(integration_tests)
    list(APPEND E_FOC_PROTOCOL_FORMAT_DIRECTORIES integration_tests)
endif()

emil_clangformat_directories(e_foc_protocol DIRECTORIES ${E_FOC_PROTOCOL_FORMAT_DIRECTORIES})
emil_folderize_all_targets()
emil_exclude_directory_from_coverage(infra/embedded-infra-lib)
emil_exclude_directory_from_coverage(infra/can-lite)
emil_exclude_directory_from_coverage(infra/numerical-toolbox)
emil_coverage_targets(DIRECTORIES .)

set(CPACK_GENERATOR "ZIP;TGZ")
set(CPACK_ARCHIVE_COMPONENT_INSTALL On)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY On)
set(CPACK_COMPONENTS_ALL e_foc_protocol)
set(CPACK_SOURCE_IGNORE_FILES ".vs/;.git/;build/")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_DEBIAN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_RPM_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)
