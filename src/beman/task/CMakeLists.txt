# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_library(beman.task STATIC)
add_library(beman::task ALIAS beman.task)

target_sources(beman.task PRIVATE task.cpp)

target_sources(
    beman.task
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/include
        FILES ${PROJECT_SOURCE_DIR}/include/beman/task/task.hpp
)

set_target_properties(beman.task PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
target_link_libraries(beman.task PUBLIC beman::execution)

install(
    TARGETS beman.task
    EXPORT beman.task
    DESTINATION
    $<$<CONFIG:Debug>:debug/>${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION $<$<CONFIG:Debug>:debug/>${CMAKE_INSTALL_BINDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
