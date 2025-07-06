# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.25)

project(
    beman.task # CMake Project Name, which is also the name of the top-level
    # targets (e.g., library, executable, etc.).
    DESCRIPTION "A Beman library task (coroutine task)"
    LANGUAGES CXX
)

enable_testing()

# [CMAKE.SKIP_TESTS]
option(
    BEMAN_TASK_BUILD_TESTS
    "Enable building tests and test infrastructure. Default: ON. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

# [CMAKE.SKIP_EXAMPLES]
option(
    BEMAN_TASK_BUILD_EXAMPLES
    "Enable building examples. Default: ON. Values: { ON, OFF }."
    ${PROJECT_IS_TOP_LEVEL}
)

include(FetchContent)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(TARGETS_EXPORT_NAME ${CMAKE_PROJECT_NAME}Targets)

FetchContent_Declare(
    execution
    # SOURCE_DIR <path-to>/execution
    GIT_REPOSITORY https://github.com/bemanproject/execution
    GIT_TAG 578c05
)
FetchContent_MakeAvailable(execution)
FetchContent_Declare(
    net
    # SOURCE_DIR <path-to>/net
    GIT_REPOSITORY https://github.com/bemanproject/net
    GIT_TAG 53a8738
)
if(NOT WIN32)
    FetchContent_MakeAvailable(net)
endif()

add_subdirectory(src/beman/task)

if(BEMAN_TASK_BUILD_TESTS)
    add_subdirectory(tests/beman/task)
endif()

if(BEMAN_TASK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# This will be used to replace @PACKAGE_cmakeModulesDir@
set(cmakeModulesDir cmake/beman)
configure_package_config_file(
    cmake/Config.cmake.in
    BemanTaskConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beman/task/
    PATH_VARS cmakeModulesDir
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/BemanTaskConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/beman/task/
    COMPONENT beman_task_development
)
