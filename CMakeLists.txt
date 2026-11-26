cmake_minimum_required(VERSION 3.20)
project(GovnoKernelHelloWorld LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Stage 1: Build-time HolyC AOT Compiler & Polyglot Bundler
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
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/09_holy_forth_lisp.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/10_holy_network_tcp.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/11_holy_neural_net.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/12_holy_ext2_vfs.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/13_holy_aes_chacha.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/14_holy_matrix_fft.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/15_holy_rtos_scheduler.HC"
    "${CMAKE_CURRENT_SOURCE_DIR}/holyc/hello_temple.HC"
)

set(POLYGLOT_INPUTS
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello_x86_64.asm"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello_2d.b93"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.fth"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.lisp"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.prolog"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.bf"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/HELLO.cbl"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.f90"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/Hello.hs"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.rs"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.zig"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.lua"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/sacred.sql"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/plasma.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.tcl"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.pl"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.rb"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.jl"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.ps1"
    "${CMAKE_CURRENT_SOURCE_DIR}/polyglot/hello.dart"
)

set(HOLYC_GEN_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated_from_holyc.cpp")
set(CURSED_ICO "${CMAKE_CURRENT_BINARY_DIR}/cursed.ico")
set(POLYGLOT_BUNDLE_HPP "${CMAKE_CURRENT_BINARY_DIR}/generated_polyglot_bundle.hpp")

add_custom_command(
    OUTPUT "${HOLYC_GEN_CPP}" "${CURSED_ICO}"
    COMMAND holyc2cpp "${HOLYC_GEN_CPP}" "${CURSED_ICO}" ${HOLYC_INPUTS}
    DEPENDS holyc2cpp ${HOLYC_INPUTS}
    COMMENT "AOT-compiling 16 HolyC modules into C++20..."
)

add_custom_command(
    OUTPUT "${POLYGLOT_BUNDLE_HPP}"
    COMMAND holyc2cpp --bundle-polyglot "${POLYGLOT_BUNDLE_HPP}" ${POLYGLOT_INPUTS}
    DEPENDS holyc2cpp ${POLYGLOT_INPUTS}
    COMMENT "Bundling 21 Polyglot language sources..."
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
    src/madness/extreme_cs.cpp
    src/madness/apocalypse_engines.cpp
    src/polyglot/polyglot_engines_part1.cpp
    src/polyglot/polyglot_engines_part2.cpp
    src/polyglot/polyglot_engines_part3.cpp
    src/gui/cursed_gui.cpp
    "${HOLYC_GEN_CPP}"
    "${POLYGLOT_BUNDLE_HPP}"
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
