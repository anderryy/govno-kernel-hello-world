# GovnoOS Kernel Builder & HolyC Apocalypse Compiler v5.0

Максимально переусложнённый компиляторный стек, микроядро и Win32 GDI-демосцена для вывода 11 символов **`Hello world`**.

## Архитектура конвейера

Каждый запуск проходит полный сквозной путь от AOT-транспиляции HolyC до исполнения машинного байткода на виртуальном CPU:

1. **Build-Time HolyC AOT Bootstrapping (`holyc/*.HC` + `src/tools/holyc2cpp_bootstrap.cpp`)**
   - Во время сборки `holyc2cpp.exe` транспилирует **9 модулей HolyC** (`01_temple_boot.HC` — GDT/IDT/PML4/ACPI; `02_holy_math_crypto.HC` — RSA, решето простых, Collatz, CRC32, FNV1a; `03_doldoc_vga_gfx.HC` — 16-цветная VGA-палитра, Брезенхэм, Мандельброт, 3D-LUT; `04_brainfuck_in_holyc.HC` — три VM внутри HolyC: Brainfuck, SUBLEQ и RPN; `05_god_oracle_dict.HC` — 64-словный оракул TempleOS и DolDoc; `06_holy_raytracer_physics.HC` — fixed-point трассировщик лучей и физика; `07_holy_compiler_sql_sort.HC` — сортировки, SQL-сканер, RLE; `08_holy_chess_minimax_ai.HC` — поиск пути 8x8, Minimax и перцептрон; `hello_temple.HC` — главная точка входа) в C++20 и генерирует иконку `cursed.ico` для `rc.exe`.
2. **Compile-Time C++20 Metaprogramming (`include/cursed_macros.hpp`)**
   - `consteval`-интерпретатор Brainfuck (`compile_time_bf_eval_h`), рекурсивные шаблоны Фибоначчи и проверки простых чисел, Коллатц-XOR расшифровка и 10-слойная обёртка `EnterpriseMonadOnion<T, 8>`.
3. **BIOS Probe & Kernel Build (`src/kernel/bios_acpi.cpp`)**
   - Чтение реального CPU через `__cpuid` и объёма ОЗУ через `GlobalMemoryStatusEx`, выделение виртуальных таблиц страниц и сборка 24 объектных модулей ядра `.o`.
4. **Многослойное обучение, эволюция и криптография (`src/madness/ml_genetics_crypto.cpp`)**
   - **Нейросеть (Backprop + SGD)**: многослойный перцептрон `11 -> 18 tanh -> 1` с нуля обучается предсказывать ASCII-коды целевой строки с выводом кривой MSE Loss в реальном времени.
   - **Генетический алгоритм**: популяция из 96 случайных строк проходит турнирный отбор, кроссинговер и мутации.
   - **RSA + Брутфорс**: шифрование публичным ключом `(N=67591, e=17)`, последующая факторизация `N` перебором делителей (`p=257, q=263`), вычисление закрытого ключа `d` и расшифровка.
   - **LZ77 -> Base64 -> Hex -> ITU Морзе**: сжатие, перекодировка в точки/тире и обратная распаковка.
5. **Runtime HolyC Compiler & VM (`src/holyc/holyc_compiler.cpp`)**
   - Лексер, парсер и стеково-регистровая виртуальная машина HolyC с поддержкой `#exe`, `Class`, `while`, цепных сравнений, DolDoc-разметки, `asm{}` и `GodTalk()` на базе `__rdtsc()`.
6. **A* Лабиринт (`src/madness/maze_astar.cpp`)**
   - Процедурная генерация подземелья `25x11` и поиск кратчайшего пути алгоритмом `A*` для доставки строки к следующему компилятору.
7. **8-этапный компилятор и SUBLEQ OISC (`src/compiler/govno_compiler.cpp`, `src/madness/subleq_oisc.cpp`)**
   - `XML -> JSON -> GovnoSQL -> GovnoScript -> #DEFINE -> goto-лексер -> AST -> деоптимизатор/IR -> GovnoLisp -> C99 -> Brainfuck -> SUBLEQ OISC (1-инструкционный процессор) -> GovnoASM + GovnoLD -> .gexe`.
8. **Блокчейн PoW, TCP/IP стек и GovnoVirtualCPU (`src/kernel/vfs_network.cpp`, `src/kernel/govno_kernel.cpp`)**
   - Майнинг блока Proof-of-Work, фрагментация `.gexe` в кадры Ethernet/IPv4/TCP/HTTP с расчётом IPv4 checksum, запись в `GovnoVFS` с проверкой CRC32 и потактовое исполнение на виртуальном CPU (`INT 0x80` -> `VGA 0xB8000`).
9. **Win32 GUI, 3D-рендерер и аудиосинтезатор (`src/gui/cursed_gui.cpp`, `src/madness/software_3d.cpp`, `src/madness/chiptune_synth.cpp`)**
   - Программный 3D-растеризатор куба + трассировка лучей зеркальной сферы на CPU, график MSE Loss, 16-цветный режим TempleOS VGA, встроенная песочница `HOLYC IDE`, рой летающих окон-спутников и PCM-синтезатор (`56k` модем + 8-bit кейген) через `winmm.lib`.

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

- Обычный GUI-режим: `GovnoKernelHelloWorld.exe`
- Консольная диагностика: `GovnoKernelHelloWorld.exe --self-test`
