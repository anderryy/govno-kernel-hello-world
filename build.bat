@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo =======================================================================
echo   GOVNO KERNEL ENTERPRISE BUILD SYSTEM v5.0 (HOLYC AOT + RAYTRACER)
echo =======================================================================

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [FATAL] vswhere.exe not found!
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -all -products * -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

if not defined VS_PATH (
    echo [FATAL] Visual Studio Build Tools not found!
    exit /b 1
)

echo [BUILD] Found Visual Studio at: %VS_PATH%
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [FATAL] vcvars64.bat failed!
    exit /b 1
)

if not exist build mkdir build

echo [STAGE 1/4] Building HolyC-to-C++ AOT Bootstrap Compiler (holyc2cpp.exe)...
cl /nologo /std:c++20 /utf-8 /EHsc /O2 /Fo:build\ /Fe:build\holyc2cpp.exe src\tools\holyc2cpp_bootstrap.cpp
if errorlevel 1 (
    echo [FATAL] Failed to compile holyc2cpp bootstrap compiler!
    exit /b 1
)

echo [STAGE 2/4] Compiling 9 HolyC modules (holyc\*.HC) into C++20 and generating cursed.ico...
build\holyc2cpp.exe build\generated_from_holyc.cpp build\cursed.ico ^
    holyc\01_temple_boot.HC ^
    holyc\02_holy_math_crypto.HC ^
    holyc\03_doldoc_vga_gfx.HC ^
    holyc\04_brainfuck_in_holyc.HC ^
    holyc\05_god_oracle_dict.HC ^
    holyc\06_holy_raytracer_physics.HC ^
    holyc\07_holy_compiler_sql_sort.HC ^
    holyc\08_holy_chess_minimax_ai.HC ^
    holyc\hello_temple.HC
if errorlevel 1 (
    echo [FATAL] HolyC AOT compilation failed!
    exit /b 1
)

echo [STAGE 3/4] Compiling cursed Windows Icon and VersionInfo resource (govno.res)...
rc /nologo /Ibuild /fo build\govno.res res\govno.rc
if errorlevel 1 (
    echo [FATAL] Resource compilation failed!
    exit /b 1
)

echo [STAGE 4/4] Compiling 13 C++20 modules + HolyC AOT output into GovnoKernelHelloWorld.exe...
cl /nologo /std:c++20 /utf-8 /EHsc /O2 ^
    /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN ^
    /Iinclude /Isrc ^
    /Fo:build\ ^
    /Fe:GovnoKernelHelloWorld.exe ^
    src\main.cpp ^
    src\compiler\govno_compiler.cpp ^
    src\kernel\bios_acpi.cpp ^
    src\kernel\vfs_network.cpp ^
    src\kernel\govno_kernel.cpp ^
    src\holyc\holyc_compiler.cpp ^
    src\madness\ml_genetics_crypto.cpp ^
    src\madness\subleq_oisc.cpp ^
    src\madness\maze_astar.cpp ^
    src\madness\chiptune_synth.cpp ^
    src\madness\software_3d.cpp ^
    src\gui\cursed_gui.cpp ^
    build\generated_from_holyc.cpp ^
    build\govno.res ^
    user32.lib gdi32.lib comctl32.lib msimg32.lib winmm.lib ^
    /link /SUBSYSTEM:WINDOWS

if errorlevel 1 (
    echo [FATAL] Final C++ linking failed!
    exit /b 1
)

echo.
echo [SUCCESS] Built GovnoKernelHelloWorld.exe v5.0 (HolyC AOT + Raytracer + A*)!
endlocal
