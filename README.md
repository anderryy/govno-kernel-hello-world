# GovnoOS Kernel Builder & Polyglot TempleOS Apocalypse v7.0

Максимально переусложнённый мультиязычный компиляторный стек, микроядро, набор вычислительно-тяжёлых CS-движков и Win32 GDI-демосцена для вывода 11 символов **`Hello world`**.

## Архитектура конвейера

1. **Build-Time HolyC AOT Bootstrapping (`holyc/*.HC` — 16 модулей)**
   - На этапе сборки `holyc2cpp.exe` транспилирует **16 модулей HolyC** (`01`..`15` + `hello_temple.HC`) в C++20 и генерирует иконку `cursed.ico`: GDT/IDT/PML4, RSA/CRC32, DolDoc VGA/Брезенхэм, встроенные BF/SUBLEQ/RPN VM, оракул TempleOS, трассировщик лучей, SQL-сканер, Chess Minimax, Forth+Lisp, IPv4/TCP, SGD+DFT, EXT2 VFS+Adler32, AES `GF(2^8)`+ChaCha, матрицы 3×3/Walsh-Hadamard и планировщик RTOS.
2. **21 Полиглот-исходник (`polyglot/*`) + Реальные C++ ВМ/Интерпретаторы (`src/polyglot/`)**
   - Вшиты в бинарник через `--bundle-polyglot` и реально исполняются при каждом запуске:
     - **x86-64 Assembly (`hello_x86_64.asm`)**, **Befunge-93 (`hello_2d.b93`)**, **Forth (`hello.fth`)**, **Common Lisp (`hello.lisp`)**, **Prolog (`hello.prolog`)**, **Brainfuck (`hello.bf`)**, **COBOL (`HELLO.cbl`)**, **Fortran 90 (`hello.f90`)**, **Haskell (`Hello.hs`)**, **Rust (`hello.rs`)**, **Zig (`hello.zig`)**, **Lua (`hello.lua`)**, **Python (`hello.py`)**, **SQL (`sacred.sql`)**, **GLSL (`plasma.frag`)**, **Tcl (`hello.tcl`)**, **Perl (`hello.pl`)**, **Ruby (`hello.rb`)**, **Julia (`hello.jl`)**, **PowerShell (`hello.ps1`)**, **Dart (`hello.dart`)**.
3. **Тяжёлая вычислительная нагрузка ЦП (`src/madness/apocalypse_engines.cpp`, `extreme_cs.cpp`, `ml_genetics_crypto.cpp`)**
   - **Сборка ядра**: CRC32-стресс на 24 модуля `.o` (по 3.2M итераций) + **Решето Эратосфена до 64 000 000**.
   - **Криптоанализ**: **брутфорс ключа AES-128** (~6.8M полных 10-раундовых расшифровок в `GF(2^8)`) + **брутфорс 18 ключей 50-битного RSA** прямым перебором делителей + **майнинг 6 блоков SHA-256 PoW** (дерево Меркла из 1 024 транзакций, сложность 21 нулевой бит, ~12+ млн хэшей).
   - **Машинное обучение и эволюция**: ансамбль **40 нейросетей (144 000 эпох Backprop/SGD)** + 24-островная эволюция по Дарвину (`POP = 896`).
   - **Численные методы и физика**:
     - Реакция-диффузия **Gray-Scott 2D PDE** (`256×256`, `3 400` шагов).
     - Прямое и обратное 2D **JPEG DCT/IDCT** сжатие `2048×2048` (`65 536` блоков `8×8` с расчётом PSNR).
     - Гравитационная задача `N-Body` (`1 400` тел × `450` шагов) + плотная СЛАУ `720×720` методом Гаусса.
     - Имитация отжига для **256-городной задачи коммивояжёра (TSP, 1.6M перестановок)**, **64M Monte-Carlo $\pi$**, длинная арифметика **`5000!`** (`16 326` цифр).
     - **Monte-Carlo Path Tracer GI** (`800×450`, `20 spp`, 3 отскока), **4K UHD** множество Мандельброта `3840×2160` (`600` итераций), **14 ферзей (`365 596` решений)**, **Game of Life `256×256×750`**, `DFT/IDFT`, **Huffman Tree** и **A\* лабиринт**.
4. **8-этапный компилятор, SUBLEQ OISC и GovnoVirtualCPU (`src/compiler/`, `src/kernel/`)**
   - `XML -> JSON -> GovnoSQL -> GovnoScript -> #DEFINE -> goto-лексер -> AST -> IR -> GovnoLisp -> C99 -> Brainfuck -> SUBLEQ -> GovnoASM + LD -> .gexe -> TCP/IP -> GovnoVFS -> VGA 0xB8000`.
5. **Win32 GUI (24 вкладки), 6 режимов графики и 8-bit аудио (`src/gui/`, `src/madness/software_3d.cpp`)**
   - 6 режимов холста: **3D Куб**, **CPU Raytracing зеркальной сферы**, **Фрактал Мандельброта**, **Плазменный Туннель**, **Волны интерференции PDE** и **ДНК 3D-спираль**.

## Сборка и запуск (Windows / MSVC 2022)

```bat
build.bat
```

- Обычный GUI-режим: `GovnoKernelHelloWorld.exe` (кнопки `[ХАРД-ПРОГОН]` и `[ЛАЙТ-РЕЖИМ]`)
- Консольная проверка: `GovnoKernelHelloWorld.exe --self-test` (быстрая проверка) и `GovnoKernelHelloWorld.exe --self-test --heavy` (полная нагрузка)
