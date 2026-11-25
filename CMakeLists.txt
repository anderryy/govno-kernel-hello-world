cmake_minimum_required(VERSION 3.20)
project(GovnoKernelHelloWorld LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Stage 1: Build-time HolyC-to-C++ AOT compiler
add_executable(holyc2cpp src/tools/holyc2cpp_bootstrap.cpp)

set(HOLYC_INPUTS
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/01_temple_boot.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/02_holy_math_crypto.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/03_doldoc_vga_gfx.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/04_brainfuck_in_holyc.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/05_god_oracle_dict.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/06_holy_raytracer_physics.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/07_holy_compiler_sql_sort.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/08_holy_chess_minimax_ai.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/hello_temple.HC"
)
set(HOLYC_GEN_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated_from_holyc.cpp")
set(CURSED_ICO "${CMAKE_CURRENT_BINARY_DIR}/cursed.ico")

# Stage 2: Transpile all holyc/*.HC modules -> generated_from_holyc.cpp + cursed.ico
add_custom_command(
    OUTPUT "${HOLYC_GEN_CPP}" "${CURSED_ICO}"
    COMMAND holyc2cpp "${HOLYC_GEN_CPP}" "${CURSED_ICO}" ${HOLYC_INPUTS}
    DEPENDS holyc2cpp ${HOLYC_INPUTS}
    COMMENT "AOT-compiling 9 HolyC modules into C++..."
)

add_executable(GovnoKernelHelloWorld WIN32
    src/main.cpp
    src/compiler/govno_compiler.cpp
    src/kernel/bios_acpi.cpp
    src/kernel/vfs_network.cpp
    src/kernel/govno_kernel.cpp
    src/holyc/holyc_compiler.cpp
    src/madness/ml_genetics_crypto.cpp
    src/madness/subleq_oisc.cpp
    src/madness/maze_astar.cpp
    src/madness/chiptune_synth.cpp
    src/madness/software_3d.cpp
    src/gui/cursed_gui.cpp
    "${HOLYC_GEN_CPP}"
    res/govno.rc
)

target_include_directories(GovnoKernelHelloWorld PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(GovnoKernelHelloWorld PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if (MSVC)
    target_compile_options(GovnoKernelHelloWorld PRIVATE /utf-8 /EHsc /O2)
endif()

target_link_libraries(GovnoKernelHelloWorld PRIVATE
    user32
    gdi32
    comctl32
    msimg32
    winmm
)
