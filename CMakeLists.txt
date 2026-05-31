# ============================================================
# XYPlot SDK — CMake Build System
# ============================================================
# Owner: Agent B (基础设施)
# 职责: 构建骨架 + CI 门禁 + 测试框架
#
# 文件所有权:
#   include/xyplot/*.h   → Agent A (只读)
#   src/*.cpp            → Agent C, D, E (GLOB 自动收集)
#   backends/**/*.cpp    → Agent E (GLOB 自动收集)
#   tests/*.cpp          → Agent B, C, D, E (GLOB 自动收集)
#   examples/**/*.cpp    → Agent F (GLOB 自动收集)
#
# 注意: 使用 GLOB 自动发现源文件，减少 Agent 间协调成本。
#       新增 .cpp 后需重新运行 cmake --build 即可。
#       如有文件需排除编译，在此文件中显式排除。
# ============================================================

cmake_minimum_required(VERSION 3.20)
project(xyplot VERSION 0.1.0 LANGUAGES CXX)

# ──── C++ Standard ────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ──── Warnings as Errors (gate quality) ────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ============================================================
# SECTION 1: Source File Discovery
# ============================================================
# Agent C: src/datatable.cpp, src/axis_system.cpp,
#           src/coordinate_transform.cpp, src/layout_engine.cpp
# Agent D: src/line_plot.cpp, src/scatter_plot.cpp,
#           src/multi_axis.cpp, src/legend_renderer.cpp,
#           src/plot_registry.cpp
# Agent E: src/plot.cpp, src/plot_interaction.cpp,
#           src/hit_test.cpp
# Agent E: backends/recording/*.cpp, backends/blend2d/*.cpp
#
# GLOB 策略: 自动收集所有源码，新增文件无需修改本文件。
# 如需排除某文件，在下方显式 list(REMOVE_ITEM ...)。
# ============================================================

file(GLOB XYPLOT_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE XYPLOT_BACKEND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/**/*.cpp
)

# Combine all library sources
set(XYPLOT_ALL_SOURCES
    ${XYPLOT_CORE_SOURCES}
    ${XYPLOT_BACKEND_SOURCES}
)

# Debug: print discovered sources during configure
message(STATUS "XYPlot core sources: ${XYPLOT_CORE_SOURCES}")
message(STATUS "XYPlot backend sources: ${XYPLOT_BACKEND_SOURCES}")
message(STATUS "XYPlot test sources: ${XYPLOT_TEST_SOURCES}")

# ============================================================
# SECTION 2: xyplot Library
# ============================================================
add_library(xyplot ${XYPLOT_ALL_SOURCES})
target_include_directories(xyplot PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# Internal headers (src/plot_impl.h, src/hit_test.h) — not installed
target_include_directories(xyplot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)
target_compile_features(xyplot PUBLIC cxx_std_17)

# ──── Optional: Blend2D dependency (if installed) ────
# find_package(blend2d QUIET)
# if(blend2d_FOUND)
#     target_link_libraries(xyplot PRIVATE blend2d::blend2d)
#     target_compile_definitions(xyplot PUBLIC XYPLOT_HAS_BLEND2D)
# endif()

# ============================================================
# SECTION 3: Testing Framework (CTest)
# ============================================================
enable_testing()

# ──── Test source discovery ────
file(GLOB XYPLOT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
)

# ──── 3a. Contract Test Gate (CRITICAL — PO 前提 2) ────
# 此目标是 CI 门禁。IRenderDevice 接口变更不兼容时编译失败。
# Owner: Agent B — 其他人只读
# ============================================================
add_executable(test_interface_contract
    tests/test_interface_contract.cpp
)
target_link_libraries(test_interface_contract PRIVATE xyplot)

add_test(NAME interface_contract_compile
         COMMAND test_interface_contract)

set_target_properties(test_interface_contract PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD OFF
    EXCLUDE_FROM_ALL OFF
)

# ──── 3b. Unit Tests (one target per test file) ────
# 每个 Agent 拥有自己的测试文件:
#   Agent C: test_axis.cpp, test_transform.cpp
#   Agent D: test_plots.cpp
#   Agent E: test_integration.cpp
#   Agent B: test_interface_contract.cpp (above)
# ============================================================
foreach(TEST_SRC ${XYPLOT_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    # Skip contract test (already configured above)
    if(TEST_NAME STREQUAL "test_interface_contract")
        continue()
    endif()

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE xyplot)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# ============================================================
# SECTION 4: Examples (Agent F domain)
# ============================================================
# 仅当对应目录存在源码时才构建
# Owner: Agent F — 文档示例 Agent
# ============================================================

# ──── 4a. Minimal Example (Recording backend) ────
file(GLOB MINIMAL_EXAMPLE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/examples/minimal/*.cpp)
if(MINIMAL_EXAMPLE_SRC)
    add_executable(xyplot_example_minimal ${MINIMAL_EXAMPLE_SRC})
    target_link_libraries(xyplot_example_minimal PRIVATE xyplot)
endif()

# ──── 4b. Qt Backend Example ────
# 需要 Qt5/Qt6 安装。如 Qt 不可用则跳过。
# file(GLOB QT_EXAMPLE_SRC examples/qt_backend/*.cpp)
# if(QT_EXAMPLE_SRC)
#     find_package(Qt6 QUIET COMPONENTS Widgets)
#     if(NOT Qt6_FOUND)
#         find_package(Qt5 QUIET COMPONENTS Widgets)
#     endif()
#     if(Qt6_FOUND OR Qt5_FOUND)
#         add_executable(xyplot_example_qt ${QT_EXAMPLE_SRC})
#         target_link_libraries(xyplot_example_qt PRIVATE xyplot
#             $<IF:$<BOOL:${Qt6_FOUND}>,Qt6::Widgets,Qt5::Widgets>)
#     endif()
# endif()

# ──── 4c. Blend2D Demo ────
# file(GLOB BLEND2D_DEMO_SRC examples/blend2d_demo/*.cpp)
# if(BLEND2D_DEMO_SRC AND blend2d_FOUND)
#     add_executable(xyplot_example_blend2d ${BLEND2D_DEMO_SRC})
#     target_link_libraries(xyplot_example_blend2d PRIVATE xyplot blend2d::blend2d)
# endif()

# ============================================================
# SECTION 5: Install Rules
# ============================================================
include(GNUInstallDirs)

install(DIRECTORY include/xyplot
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(TARGETS xyplot
    EXPORT xyplotTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT xyplotTargets
    NAMESPACE xyplot::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xyplot
)

# ============================================================
# SECTION 6: Developer Convenience Targets
# ============================================================

# ──── gate-check target (equivalent to scripts/gate-check.sh) ────
add_custom_target(gate-check
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_interface_contract
    COMMAND ctest -R interface_contract_compile --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running interface contract gate check..."
)

# ──── test-all target (run all tests) ────
add_custom_target(test-all
    COMMAND ctest --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)

# ──── Print discovered sources (debug) ────
add_custom_target(show-sources
    COMMAND ${CMAKE_COMMAND} -E echo "=== Core Sources ==="
    COMMAND ${CMAKE_COMMAND} -E echo "${XYPLOT_CORE_SOURCES}"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Backend Sources ==="
    COMMAND ${CMAKE_COMMAND} -E echo "${XYPLOT_BACKEND_SOURCES}"
    COMMAND ${CMAKE_COMMAND} -E echo "=== Test Sources ==="
    COMMAND ${CMAKE_COMMAND} -E echo "${XYPLOT_TEST_SOURCES}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Discovered source files:"
)

# ============================================================
# SECTION 7: CPack (Optional — for future release packaging)
# ============================================================
# set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
# set(CPACK_GENERATOR "ZIP;TGZ")
# include(CPack)

# ============================================================
# End of CMakeLists.txt
# ============================================================
