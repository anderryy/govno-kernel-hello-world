#include "kernel/govno_kernel.hpp"
#include "kernel/bios_acpi.hpp"
#include "kernel/vfs_network.hpp"
#include "compiler/govno_compiler.hpp"
#include "holyc/holyc_compiler.hpp"
#include "holyc_aot_bridge.hpp"
#include "madness/ml_genetics_crypto.hpp"
#include "madness/maze_astar.hpp"
#include "madness/extreme_cs.hpp"
#include "polyglot/polyglot_engines.hpp"
#include "cursed_macros.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace govno::kernel {

namespace {

class VirtualMemorySubsystem {
    std::vector<uint8_t> physical_frames_;
    uint32_t allocated_kb_ = 0;

public:
    VirtualMemorySubsystem() : physical_frames_(32768, 0) {}
    uint64_t alloc_kernel_pages(uint32_t count) {
        for (uint32_t i = 0; i < count && (i + 64) < physical_frames_.size(); ++i) {
            physical_frames_[i + 64] = 1;
            allocated_kb_ += 4;
        }
        return 0xFFFF880000000000ULL + static_cast<uint64_t>(allocated_kb_) * 1024ULL;
    }
    uint32_t total_allocated_kb() const { return allocated_kb_; }
};

struct VirtualInode {
    std::string path;
    std::vector<uint8_t> data;
    uint32_t crc32 = 0;
};

class GovnoVFS {
    std::unordered_map<std::string, VirtualInode> inodes_;

public:
    void mount_root() {
        write_file("/etc/govno.conf", {'O','K','=','1'});
        write_file("/boot/vmlinuz-govno-0.69", {'E','L','F','G','O','V','N','O'});
    }
    void write_file(const std::string& path, const std::vector<uint8_t>& bytes) {
        VirtualInode node{path, bytes, govno::compiler::compute_crc32(bytes.data(), bytes.size())};
        inodes_[path] = std::move(node);
    }
    const VirtualInode& read_file_checked(const std::string& path) const {
        const auto& node = inodes_.at(path);
        if (govno::compiler::compute_crc32(node.data.data(), node.data.size()) != node.crc32) {
            throw std::runtime_error("GovnoVFS ECC error in " + path);
        }
        return node;
    }
};

class GovnoVirtualCPU {
    std::array<uint8_t, 4096> memory_tape_{};
    uint64_t tape_ptr_ = 0, rip_ = 0, cycles_ = 0, syscalls_ = 0;
    uint64_t rax_ = 0, rbx_ = 0xDEADBEEF, rcx_ = 0xCAFEBABE, rsp_ = 0x00007FFFFFFFE000ULL;
    bool halted_ = false;

    static uint32_t read_u32_le(const std::vector<uint8_t>& buf, size_t off) {
        return static_cast<uint32_t>(buf[off]) |
               (static_cast<uint32_t>(buf[off + 1]) << 8) |
               (static_cast<uint32_t>(buf[off + 2]) << 16) |
               (static_cast<uint32_t>(buf[off + 3]) << 24);
    }

public:
    void execute_gexe(
        const std::vector<uint8_t>& gexe_image,
        uint32_t ram_kb,
        size_t subleq_cyc,
        size_t holyc_cyc,
        std::atomic<bool>& cancel_flag,
        const std::function<void(int ms)>& sleep_cb,
        const std::function<void(char ch)>& vga_write_cb
    ) {
        uint32_t entry = read_u32_le(gexe_image, 8);
        uint32_t code_sz = read_u32_le(gexe_image, 12);
        std::vector<uint8_t> code(gexe_image.begin() + 16, gexe_image.begin() + 16 + code_sz);
        uint32_t expected_crc = read_u32_le(gexe_image, 16 + code_sz);
        if (expected_crc != govno::compiler::compute_crc32(code.data(), code.size())) {
            throw std::runtime_error(".GEXE CRC32 mismatch!");
        }

        rip_ = entry;
        halted_ = false;
        while (!halted_ && !cancel_flag.load() && rip_ < code.size() && cycles_ < 500000) {
            uint8_t op = code[static_cast<size_t>(rip_)];
            std::string op_name = "UNK";
            ++cycles_;

            switch (static_cast<govno::compiler::GovnoOpcode>(op)) {
                case govno::compiler::GovnoOpcode::OP_ADD_MEM: {
                    uint8_t imm = code[static_cast<size_t>(rip_ + 1)];
                    memory_tape_[static_cast<size_t>(tape_ptr_)] += imm;
                    rax_ = memory_tape_[static_cast<size_t>(tape_ptr_)];
                    rip_ += 2; op_name = "ADD_MEM +" + std::to_string(imm);
                    break;
                }
                case govno::compiler::GovnoOpcode::OP_SUB_MEM: {
                    uint8_t imm = code[static_cast<size_t>(rip_ + 1)];
                    memory_tape_[static_cast<size_t>(tape_ptr_)] -= imm;
                    rax_ = memory_tape_[static_cast<size_t>(tape_ptr_)];
                    rip_ += 2; op_name = "SUB_MEM -" + std::to_string(imm);
                    break;
                }
                case govno::compiler::GovnoOpcode::OP_PTR_INC: {
                    uint8_t imm = code[static_cast<size_t>(rip_ + 1)];
                    tape_ptr_ = (tape_ptr_ + imm) % memory_tape_.size();
                    rbx_ = tape_ptr_;
                    rip_ += 2; op_name = "PTR_INC +" + std::to_string(imm);
                    break;
                }
                case govno::compiler::GovnoOpcode::OP_PTR_DEC: {
                    uint8_t imm = code[static_cast<size_t>(rip_ + 1)];
                    tape_ptr_ = (tape_ptr_ + memory_tape_.size() - imm) % memory_tape_.size();
                    rbx_ = tape_ptr_;
                    rip_ += 2; op_name = "PTR_DEC -" + std::to_string(imm);
                    break;
                }
                case govno::compiler::GovnoOpcode::OP_JZ: {
                    uint32_t addr = read_u32_le(code, static_cast<size_t>(rip_ + 1));
                    op_name = "JZ 0x" + std::to_string(addr);
                    if (memory_tape_[static_cast<size_t>(tape_ptr_)] == 0) rip_ = addr;
                    else rip_ += 5;
                    break;
                }
                case govno::compiler::GovnoOpcode::OP_JNZ: {
                    uint32_t addr = read_u32_le(code, static_cast<size_t>(rip_ + 1));
                    op_name = "JNZ 0x" + std::to_string(addr);
                    if (memory_tape_[static_cast<size_t>(tape_ptr_)] != 0) rip_ = addr;
                    else rip_ += 5;
                    break;
                }
                case govno::compiler::GovnoOpcode::OP_SYSCALL: {
                    ++syscalls_;
                    char ch = static_cast<char>(memory_tape_[static_cast<size_t>(tape_ptr_)]);
                    rax_ = static_cast<uint8_t>(ch);
                    op_name = std::string("INT 0x80 ('") + (ch >= 32 ? ch : '?') + "')";
                    vga_write_cb(ch);
                    rip_ += 2;
                    sleep_cb(120);
                    break;
                }
                case govno::compiler::GovnoOpcode::OP_HALT:
                    op_name = "HALT";
                    halted_ = true;
                    rip_ += 1;
                    break;
                default:
                    throw std::runtime_error("CPU #UD!");
            }

            if ((cycles_ % 3 == 0) || op == static_cast<uint8_t>(govno::compiler::GovnoOpcode::OP_SYSCALL) || halted_) {
                auto& st = g_state();
                std::lock_guard<std::mutex> lock(st.mtx);
                st.cpu.rip = rip_; st.cpu.rax = rax_; st.cpu.rbx = rbx_; st.cpu.rcx = rcx_; st.cpu.rsp = rsp_;
                st.cpu.tape_ptr = tape_ptr_;
                st.cpu.tape_val = memory_tape_[static_cast<size_t>(tape_ptr_)];
                st.cpu.cycles = cycles_;
                st.cpu.subleq_cycles = subleq_cyc;
                st.cpu.holyc_cycles = holyc_cyc;
                st.cpu.syscalls = syscalls_;
                st.cpu.ram_allocated_kb = ram_kb;
                st.cpu.halted = halted_;
                st.cpu.current_op = op_name;
                for (size_t i = 0; i < 16; ++i) st.cpu.tape_preview[i] = memory_tape_[i];
            }
        }
    }
};

} // namespace

void run_entire_kernel_and_vm_pipeline(std::atomic<bool>& cancel_flag, bool turbo_mode) {
    const auto t_start = std::chrono::steady_clock::now();

    auto sleep_step = [&](int ms) {
        if (cancel_flag.load()) return;
        bool is_turbo = turbo_mode;
        {
            std::lock_guard<std::mutex> lock(g_state().mtx);
            is_turbo = g_state().turbo_mode;
        }
        if (is_turbo || ms <= 0) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    };

    HardwareProbeInfo hw = probe_real_hardware_for_comedy();

    {
        auto& st = g_state();
        std::lock_guard<std::mutex> lock(st.mtx);
        st.stage = PipelineStage::STAGE_1_TEMPLATE_ORACLE;
        st.status_banner = "Запуск HolyC AOT модуля (скомпилированного при сборке .exe) + BIOS...";
        st.kernel_progress = 0;
        st.ml_crypto_progress = 0;
        st.compiler_progress = 0;
        st.driver_net_progress = 0;
        st.elapsed_ms = 0;
        st.completed = false;
        st.failed = false;
        st.ml_loss_history.clear();
        st.ml_live_prediction.clear();
        st.kernel_logs.clear();
        st.final_vga_output.clear();
        st.vga_line.fill(VGACell{0});
        st.cpu = CpuSnapshot{};
        st.cpu.cpu_brand = hw.cpu_brand;
        st.artifacts = CompilerArtifacts{};
    }

    append_kernel_log("==========================================================================");
    append_kernel_log("  GOVNO-OS TEMPLE APOCALYPSE v5.0 (HOLYC AOT + A* + RAYTRACER + SUBLEQ)");
    append_kernel_log("  CPU: " + hw.cpu_brand + " | RAM: " + std::to_string(hw.total_ram_mb) + " МБ");
    append_kernel_log("==========================================================================");

    // 0. Execute native code AHEAD-OF-TIME compiled from all 9 holyc/*.HC modules during build.bat!
    auto holyc_aot = govno::holyc_aot::run_compiled_holyc_aot();
    append_kernel_log("[HOLYC-AOT] Вызван нативный код из 9 скомпилированных модулей holyc/*.HC!");
    {
        std::istringstream aot_iss(holyc_aot.aot_log);
        std::string aln;
        while (std::getline(aot_iss, aln)) {
            if (!aln.empty() && aln.back() == '\r') aln.pop_back();
            if (!aln.empty()) append_kernel_log("  " + aln);
        }
    }
    append_kernel_log("[HOLYC-AOT] Выход AOT (HOLY_SHIFT=" + std::to_string(holyc_aot.magic_power) +
                      "): \"" + holyc_aot.emitted_text + "\"");
    sleep_step(180);

    // 1. BIOS + VMM + Build 24 Kernel .o files
    VirtualMemorySubsystem vmm;
    vmm.alloc_kernel_pages(1536);
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_2_BIOS_AND_KERNEL_BUILD;
        g_state().status_banner = "Сборка 24 модулей ядра GovnoOS из исходников...";
        g_state().cpu.ram_allocated_kb = vmm.total_allocated_kb();
        g_state().cpu.page_faults = 13;
    }

    size_t built_modules = simulate_kernel_source_tree_build(
        cancel_flag,
        [&](int pct, const std::string& msg) {
            append_kernel_log(msg);
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().kernel_progress = pct;
        },
        sleep_step
    );
    if (cancel_flag.load()) return;

    // 2. Real SGD Neural Network Training seeded by HolyC AOT output!
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_3_NEURAL_NET_TRAINING;
        g_state().status_banner = "Обучение нейросети с нуля методом SGD (обучается на выходе HolyC AOT)...";
    }
    auto nn = govno::madness::train_hello_world_neural_net(
        holyc_aot.emitted_text,
        cancel_flag,
        [&](int pct, float loss, const std::string& pred, const std::string& msg) {
            append_kernel_log(msg);
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().ml_crypto_progress = (pct * 25) / 100;
            g_state().ml_loss_history.push_back(loss);
            g_state().ml_live_prediction = pred;
        },
        sleep_step
    );
    if (cancel_flag.load()) return;

    // 3. Darwin Genetic Evolution
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_4_GENETIC_EVOLUTION;
        g_state().status_banner = "Дарвинская эволюция 96 мутантов в '" + nn.predicted_string + "'...";
    }
    auto ga = govno::madness::evolve_hello_world_genetics(
        nn.predicted_string,
        cancel_flag,
        [&](int pct, const std::string& msg) {
            append_kernel_log(msg);
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().ml_crypto_progress = 25 + (pct * 25) / 100;
        },
        sleep_step
    );
    if (cancel_flag.load()) return;

    // 4. Toy RSA Encryption + Brute-force Factorization Crack
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_5_RSA_CRYPTO_AND_CRACK;
        g_state().status_banner = "Шифрование RSA и брутфорс-взлом собственного ключа...";
    }
    auto rsa = govno::madness::rsa_encrypt_and_crack(
        ga.evolved_string,
        [&](int pct, const std::string& msg) {
            append_kernel_log(msg);
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().ml_crypto_progress = 50 + (pct * 25) / 100;
        },
        sleep_step
    );

    // 5. LZ77 -> Base64 -> Hex -> ITU Morse Code -> Decode Roundtrip
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_6_LZ77_BASE_MORSE;
        g_state().status_banner = "Сжатие LZ77 -> Base64 -> Азбука Морзе -> Декодирование...";
    }
    auto morse = govno::madness::run_lz77_base64_morse_roundtrip(
        rsa.decrypted_string,
        [&](int pct, const std::string& msg) {
            append_kernel_log(msg);
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().ml_crypto_progress = 75 + (pct * 25) / 100;
        },
        sleep_step
    );

    // 6. Runtime HolyC Compiler + #exe + VM + GodTalk!
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_7_HOLYC_TEMPLEOS_COMPILER;
        g_state().status_banner = "Runtime-компиляция HolyC (#exe + Class + asm{} + GodTalk)...";
    }
    auto hc = govno::holyc::compile_and_run_holyc_stage(
        morse.recovered_string,
        cancel_flag,
        [&](int pct, const std::string& msg) {
            append_kernel_log(msg);
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().compiler_progress = (pct * 16) / 100;
        },
        sleep_step
    );
    if (cancel_flag.load()) return;

    // 6B. Route payload through procedural dungeon labyrinth via A* pathfinding!
    auto maze = govno::madness::route_payload_through_astar_maze(hc.emitted_output);
    append_kernel_log("[A*-LABYRINTH] Строка сбежала из лабиринта 25x11 за " +
                      std::to_string(maze.path_length) + " шагов (раскрыто узлов: " +
                      std::to_string(maze.nodes_expanded) + ")");
    sleep_step(80);

    // 6C. Execute all 15 bundled polyglot languages (ASM, Befunge-93, Forth, Lisp, Prolog, BF, COBOL, F90, HS, Rust, Zig, Lua, Py, SQL, GLSL)
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().status_banner = "Запуск 15 полиглот-движков (x86-64, Befunge-93, Forth, Lisp, Prolog, COBOL, Rust, Zig...)";
    }
    auto poly = govno::polyglot::run_all_polyglot_engines(maze.delivered_payload);
    append_kernel_log("[POLYGLOT-15] Исполнено " + std::to_string(poly.results.size()) +
                      " языков (" + std::to_string(poly.total_polyglot_bytes) + " Б, " +
                      std::to_string(poly.total_steps) + " шагов VM) -> \"" + poly.final_payload + "\"");

    // 6D. Extreme CS codecs & solvers: DFT -> Huffman -> Sudoku+8Queens -> Game of Life + Floyd-Warshall
    auto dft = govno::madness::dft_idft_roundtrip(poly.final_payload);
    auto huff = govno::madness::huffman_compress_decompress(dft.ok ? dft.output : "DFT_FAIL");
    auto sud = govno::madness::solve_sudoku_and_nqueens(huff.ok ? huff.output : "HUFF_FAIL");
    auto life = govno::madness::game_of_life_and_graph(sud.ok ? sud.output : "SUD_FAIL");
    std::string cs_verified_out = life.ok ? life.output : "LIFE_FAIL";
    append_kernel_log("[DSP/ALGO] DFT+IDFT -> Huffman Tree -> Sudoku+8Queens(92) -> GameOfLife(40gen)+FloydWarshall -> \"" +
                      cs_verified_out + "\"");
    sleep_step(80);

    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().cpu.god_words = hc.god_oracle_words;
        g_state().cpu.holyc_cycles = hc.vm_cycles;
    }

    // 7. 8-Stage Multi-Language Compiler taking post-polyglot & CS-verified payload!
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_8_XML_JSON_SQL;
        g_state().status_banner = "Компиляция: XML/SQL -> GovnoScript -> AST -> Lisp -> C99 -> BF -> SUBLEQ -> ASM...";
    }

    CompilerArtifacts compiled;
    compiled.ml_genetics_dump = nn.report + ga.report;
    compiled.rsa_lz77_morse_dump = rsa.report + morse.report;
    compiled.holyc_aot_dump =
        "=== BUILD-TIME HOLYC AOT (from 13 modules holyc/*.HC) ===\r\n" +
        holyc_aot.aot_log + "\r\nEmitted AOT String: \"" + holyc_aot.emitted_text + "\"\r\n" +
        "HOLY_SHIFT (2`3) = " + std::to_string(holyc_aot.magic_power) + "\r\n\r\n" +
        "=== TRANSPILED C++ GENERATED AT BUILD TIME ===\r\n" +
        govno::holyc_aot::get_holyc_transpiled_cpp_preview();
    compiled.holyc_source_dump = hc.raw_and_expanded_source;
    compiled.holyc_vm_dump = hc.vm_and_doldoc_report;
    compiled.astar_maze_dump = maze.ascii_map_dump;
    compiled.polyglot_summary_dump = poly.combined_summary;
    compiled.x86_asm_dump = poly.x86_trace;
    compiled.befunge_2d_dump = poly.befunge_trace;
    compiled.lisp_forth_dump = poly.lisp_forth_trace;
    compiled.prolog_cbl_f90_dump = poly.prolog_cbl_f90_trace;
    compiled.systems_lang_dump = poly.systems_lang_trace;
    compiled.dft_huffman_dump = dft.dump + "\r\n" + huff.dump;
    compiled.sudoku_life_dump = sud.dump + "\r\n" + life.dump;
    compiled.polyglot_languages_count = poly.results.size();
    compiled.polyglot_total_bytes = poly.total_polyglot_bytes;
    compiled.polyglot_total_steps = poly.total_steps;
    compiled.astar_nodes_expanded = maze.nodes_expanded;
    compiled.holyc_aot_shift = holyc_aot.magic_power;
    compiled.holyc_opcodes_count = hc.opcodes_count;
    compiled.holyc_vm_cycles = hc.vm_cycles;
    compiled.god_oracle_words = hc.god_oracle_words;
    compiled.kernel_modules_built = built_modules;
    compiled.ml_epochs = nn.epochs_run;
    compiled.ml_final_loss = nn.final_mse;
    compiled.genetic_generations = ga.generations;
    compiled.rsa_modulus = rsa.modulus_n;

    bool ok = govno::compiler::run_full_compiler_pipeline(
        cs_verified_out,
        compiled,
        [&](int pct, const std::string& msg) {
            append_kernel_log(msg);
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().compiler_progress = 16 + (pct * 84) / 100;
            if (pct < 20) g_state().stage = PipelineStage::STAGE_8_XML_JSON_SQL;
            else if (pct < 55) g_state().stage = PipelineStage::STAGE_9_PREPROCESSOR_LEXER_AST;
            else if (pct < 85) g_state().stage = PipelineStage::STAGE_10_IR_LISP_C_BRAINFUCK;
            else if (pct < 95) g_state().stage = PipelineStage::STAGE_11_SUBLEQ_OISC;
            else g_state().stage = PipelineStage::STAGE_12_ASM_LINKER;
        },
        sleep_step
    );
    if (!ok || cancel_flag.load()) {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().failed = true;
        g_state().stage = PipelineStage::STAGE_ERROR;
        return;
    }

    // 8. Blockchain Proof-of-Work + Ethernet/IPv4/TCP/HTTP Delivery
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_13_BLOCKCHAIN_AND_TCPIP;
        g_state().status_banner = "Майнинг блока GovnoChain + отправка .gexe по виртуальному TCP/IP...";
    }
    auto pow = mine_hello_world_blockchain_block(
        compiled.gexe_bytecode,
        [&](const std::string& msg) { append_kernel_log(msg); }
    );
    compiled.pow_nonce = pow.nonce;
    compiled.pow_block_hash = pow.block_hash_hex;
    compiled.gexe_and_blockchain_dump = pow.ledger_dump + "\r\n" + govno::compiler::format_hex_dump(compiled.gexe_bytecode);
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().cpu.pow_hashes = pow.hashes_computed;
        g_state().driver_net_progress = 15;
    }

    auto net = transmit_gexe_over_virtual_tcpip(
        compiled.gexe_bytecode,
        cancel_flag,
        [&](int pct, const std::string& msg) {
            append_kernel_log(msg);
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().driver_net_progress = 15 + (pct * 35) / 100;
            g_state().cpu.packets_sent += 1;
        },
        sleep_step
    );
    if (cancel_flag.load()) return;

    compiled.tcp_packets_dump = net.packet_trace_dump;
    compiled.tcp_packet_count = net.packet_count;
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().artifacts = compiled;
    }

    // 9. Kernel Drivers + VFS + GovnoVirtualCPU Execution
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().stage = PipelineStage::STAGE_14_DRIVERS_AND_VM_EXEC;
        g_state().status_banner = "Загрузка драйверов ядра, VFS и запуск GovnoCPU VM...";
    }

    const std::pair<const char*, const char*> drivers[] = {
        {"pci_toaster_acpi.sys",       "USB-тостер инициализирован (температура: 420°C)"},
        {"templeos_ring0_shim.sys",    "Отключена защита памяти ради духа Терри Дэвиса"},
        {"chrome_ram_eater_guard.sys", "OOM-Killer: Chrome пристрелен ради 11 символов"},
        {"astar_dungeon_driver.sys",   "Драйвер побега из лабиринта включён"},
        {"raytracer_cpu_burner.sys",   "Включена трассировка лучей на CPU для зеркального шара"},
        {"vga_comic_sans_2d_accel.sys","Аппаратное ускорение Comic Sans MS включено"}
    };
    int drv_idx = 0;
    for (const auto& [name, note] : drivers) {
        if (cancel_flag.load()) return;
        ++drv_idx;
        vmm.alloc_kernel_pages(128);
        append_kernel_log(std::string("[INSMOD] ") + name + " -> " + note);
        {
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().driver_net_progress = 50 + (drv_idx * 45) / 6;
            g_state().cpu.ram_allocated_kb = vmm.total_allocated_kb();
        }
        sleep_step(90);
    }

    GovnoVFS vfs;
    vfs.mount_root();
    vfs.write_file("/bin/hello_world.gexe", net.delivered_payload);
    const auto& inode = vfs.read_file_checked("/bin/hello_world.gexe");
    append_kernel_log("[GOVNO-VFS] /bin/hello_world.gexe загружен из TCP-потока, CRC32 валиден.");

    GovnoVirtualCPU cpu;
    size_t vga_cursor = 0;
    cpu.execute_gexe(
        inode.data,
        vmm.total_allocated_kb(),
        compiled.subleq_cycles,
        compiled.holyc_vm_cycles,
        cancel_flag,
        sleep_step,
        [&](char ch) {
            std::lock_guard<std::mutex> lock(g_state().mtx);
            g_state().final_vga_output.push_back(ch);
            if (vga_cursor < g_state().vga_line.size()) {
                VGACell cell{};
                cell.parts.ch = static_cast<uint8_t>(ch);
                cell.parts.attr = static_cast<uint8_t>(0x0A + (vga_cursor % 5));
                g_state().vga_line[vga_cursor++] = cell;
            }
            std::ostringstream hs;
            hs << std::hex << std::setw(2) << std::setfill('0') << (int)(uint8_t)ch;
            g_state().kernel_logs.push_back(
                "[INT 0x80] SYS_WRITE(0x" + hs.str() + ") -> VGA[0xB8000+" +
                std::to_string((vga_cursor - 1) * 2) + "] = '" + ch + "'"
            );
        }
    );

    const auto t_end = std::chrono::steady_clock::now();
    const uint64_t ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count()
    );

    std::string final_str;
    {
        std::lock_guard<std::mutex> lock(g_state().mtx);
        g_state().elapsed_ms = ms;
        g_state().kernel_progress = 100;
        g_state().ml_crypto_progress = 100;
        g_state().compiler_progress = 100;
        g_state().driver_net_progress = 100;
        g_state().completed = !cancel_flag.load();
        g_state().stage = PipelineStage::STAGE_15_GRAND_FINALE_HELLO_WORLD;
        g_state().status_banner = "ТРИУМФ v6.0! 'Hello world' ПРОВЕРЕН ЧЕРЕЗ 13 HOLYC + 15 ЯЗЫКОВ + SHA-256 + DFT + ХАФФМАН + СУДОКУ + LIFE + 3D!";
        final_str = g_state().final_vga_output;
    }

    append_kernel_log("==========================================================================");
    append_kernel_log("[VICTORY] VGA 0xB8000 результат: \"" + final_str + "\" за " + std::to_string(ms) + " мс!");
    append_kernel_log("==========================================================================");
}

} // namespace govno::kernel
