# GovnoOS Kernel Builder & Polyglot TempleOS Apocalypse v6.0

Максимально переусложнённый мультиязычный компиляторный стек, микроядро, набор CS-движков и Win32 GDI-демосцена для вывода 11 символов **`Hello world`**.

## Архитектура конвейера

Каждый запуск проходит полный сквозной путь от AOT-транспиляции HolyC и исполнения 15 языков до криптографии, DSP и виртуального процессора:

1. **Build-Time HolyC AOT Bootstrapping (`holyc/*.HC` — 13 модулей + `src/tools/holyc2cpp_bootstrap.cpp`)**
   - Во время сборки `holyc2cpp.exe` транспилирует **13 модулей HolyC** (`01_temple_boot.HC`..`12_holy_ext2_vfs.HC` + `hello_temple.HC`) в C++20 и генерирует иконку `cursed.ico` для `rc.exe`.
   - Покрывает GDT/IDT/PML4, RSA/CRC32/FNV1a, DolDoc VGA/Брезенхэм, встроенные BF/SUBLEQ/RPN-VM на HolyC, 64-словный оракул TempleOS, fixed-point трассировку лучей, SQL-сканер, Chess Minimax + перцептрон, Forth+Lisp на HolyC, RFC-1071 IPv4/TCP, SGD+DFT и EXT2 VFS + Adler32.
2. **15 Полиглот-исходников (`polyglot/*`) + Реальные C++ Интерпретаторы/ВМ (`src/polyglot/`)**
   - На этапе сборки `holyc2cpp --bundle-polyglot` вшивает в бинарник исходники 15 языков:
     - **x86-64 Assembly (`hello_x86_64.asm`)** — эмулятор регистров (`rax..r15`), флагов `ZF/SF`, стека и Linux `sys_write`/`sys_exit`.
     - **Befunge-93 (`hello_2d.b93`)** — 2D-интерпретатор сетки с движением IP во всех направлениях и стеком.
     - **Forth (`hello.fth`)** — словарный компилятор (`: ... ;`), `VARIABLE`/`CONSTANT`, цикл `DO ... LOOP` и `EMIT`.
     - **Common Lisp (`hello.lisp`)** — рекурсивный парсер S-выражений (`defun`, `let`, `if`, `list`, `code-char`).
     - **Prolog (`hello.prolog`)** — SLD-резолюция с унификацией фактов, предикатами `is`/`>=`/`=<` и бэктрекингом.
     - **Brainfuck (`hello.bf`)**, **COBOL (`HELLO.cbl`)**, **Fortran 90 (`hello.f90`)**, **Haskell (`Hello.hs`)**, **Rust (`hello.rs`)**, **Zig (`hello.zig`)**, **Lua (`hello.lua`)**, **Python (`hello.py`)**, **SQL (`sacred.sql`)**, **GLSL (`plasma.frag`)**.
3. **Алгоритмический хаос (`src/madness/extreme_cs.cpp` & `maze_astar.cpp` & `ml_genetics_crypto.cpp`)**
   - **Дискретное преобразование Фурье (DFT -> IDFT)**: перевод ASCII в комплексный частотный спектр и обратное восстановление.
   - **Бинарный кодек Хаффмана**: построение дерева по частотам и побитовое сжатие/декодирование.
   - **Судоку (Arto Inkala 9x9) + Задача о 8 ферзях (92 решения)** с привязкой инвариантов.
   - **Conway's Game of Life (32x16, 40 поколений) + Роутинг Флойда–Уоршелла**.
   - **Нейросеть Backprop/SGD**, **Генетический алгоритм**, **RSA-факторизация**, **LZ77 + Base64 + Морзе**, **A\* Лабиринт 25x11**.
4. **Runtime HolyC VM, 8-этапный компилятор и SUBLEQ OISC (`src/holyc/`, `src/compiler/`, `src/madness/subleq_oisc.cpp`)**
   - `XML -> JSON -> GovnoSQL -> GovnoScript -> #DEFINE -> goto-лексер -> AST -> IR -> GovnoLisp -> C99 -> Brainfuck -> SUBLEQ OISC -> GovnoASM + LD -> .gexe`.
5. **FIPS-180-4 SHA-256 PoW, TCP/IP и GovnoVirtualCPU (`src/kernel/vfs_network.cpp`, `src/kernel/govno_kernel.cpp`)**
   - Настоящий майнинг блока **SHA-256 Proof-of-Work** с нулевым префиксом, фрагментация `.gexe` в кадры Ethernet/IPv4/TCP/HTTP, проверка CRC32 в `GovnoVFS` и потактовое исполнение на виртуальном CPU (`INT 0x80 -> VGA 0xB8000`).
6. **Win32 GUI (20 вкладок), 4 режима графики и 8-bit аудио (`src/gui/cursed_gui.cpp`, `src/madness/software_3d.cpp`)**
   - Переключаемые режимы холста: **3D Куб**, **CPU Raytracing зеркальной сферы с тенями**, **Фрактал Мандельброта (Seahorse Valley zoom)** и **Демосцена Плазменный Туннель**, плюс живой редактор `HOLYC IDE`, 16-цветный TempleOS VGA, летающий рой окон и аудиосинтезатор `56k` / chiptune.

## Сборка (Windows / MSVC 2022)

```bat
build.bat
```

Или через CMake:

```powershell
cmake -B out -G "Visual Studio 17 2022" -A x64
cmake --build out --config Release
```

## Запуск

- Готовый бинарник `GovnoKernelHelloWorld.exe` находится в разделе **Releases**.
- Обычный GUI-режим: `GovnoKernelHelloWorld.exe`
- Консольная диагностика: `GovnoKernelHelloWorld.exe --self-test`
