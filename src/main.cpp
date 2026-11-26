#include "gui/cursed_gui.hpp"
#include "kernel/govno_kernel.hpp"
#include "kernel_types.hpp"
#include "cursed_macros.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <io.h>
#include <string>
#include <windows.h>

namespace {

int run_headless_self_test() {
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD ftype = (h_out && h_out != INVALID_HANDLE_VALUE) ? GetFileType(h_out) : FILE_TYPE_UNKNOWN;
    if (ftype == FILE_TYPE_PIPE || ftype == FILE_TYPE_DISK) {
        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(h_out), _O_TEXT);
        if (fd != -1) {
            _dup2(fd, _fileno(stdout));
            setvbuf(stdout, nullptr, _IONBF, 0);
        }
    } else if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* f_out = nullptr;
        freopen_s(&f_out, "CONOUT$", "w", stdout);
    }
    SetConsoleOutputCP(CP_UTF8);

    std::atomic<bool> cancel{false};
    govno::kernel::run_entire_kernel_and_vm_pipeline(cancel, true);

    std::string result, cpu_brand, god_words, pow_hash, poly_summary;
    size_t kernel_mods = 0, ml_epochs = 0, ga_gens = 0, hc_ops = 0, hc_cyc = 0;
    size_t astar_nodes = 0, subleq_cyc = 0, bf_len = 0, gexe_len = 0, tcp_pkts = 0;
    size_t poly_langs = 0, poly_bytes = 0, poly_steps = 0;
    int64_t holyc_aot_shift = 0;
    float ml_loss = 0.0f;
    uint64_t rsa_n = 0, pow_hashes = 0, vm_cycles = 0;

    {
        std::lock_guard<std::mutex> lock(govno::g_state().mtx);
        const auto& st = govno::g_state();
        result = st.final_vga_output;
        cpu_brand = st.cpu.cpu_brand;
        god_words = st.artifacts.god_oracle_words;
        pow_hash = st.artifacts.pow_block_hash;
        poly_summary = st.artifacts.polyglot_summary_dump;
        kernel_mods = st.artifacts.kernel_modules_built;
        ml_epochs = st.artifacts.ml_epochs;
        ml_loss = st.artifacts.ml_final_loss;
        ga_gens = st.artifacts.genetic_generations;
        rsa_n = st.artifacts.rsa_modulus;
        holyc_aot_shift = st.artifacts.holyc_aot_shift;
        hc_ops = st.artifacts.holyc_opcodes_count;
        hc_cyc = st.artifacts.holyc_vm_cycles;
        astar_nodes = st.artifacts.astar_nodes_expanded;
        subleq_cyc = st.artifacts.subleq_cycles;
        bf_len = st.artifacts.bf_length;
        gexe_len = st.artifacts.gexe_size_bytes;
        tcp_pkts = st.artifacts.tcp_packet_count;
        pow_hashes = st.cpu.pow_hashes;
        vm_cycles = st.cpu.cycles;
        poly_langs = st.artifacts.polyglot_languages_count;
        poly_bytes = st.artifacts.polyglot_total_bytes;
        poly_steps = st.artifacts.polyglot_total_steps;
    }

    std::printf(
        "GOVNO-KERNEL TEMPLE-OS v6.0 (13x HOLYC AOT + 15 POLYGLOT VMs + SHA256 + DFT/HUFFMAN + 4D GFX) SELF-TEST\n"
        "CPU: %s\n"
        "Result: \"%s\"\n"
        "HolyC AOT HOLY_SHIFT (2`3): %lld\n"
        "Polyglot Suite: %zu languages (%zu B, %zu VM steps)\n"
        "%s"
        "Kernel .o modules: %zu\n"
        "Neural Net SGD epochs: %zu (final MSE=%.6f)\n"
        "Genetic Algorithm generations: %zu\n"
        "RSA modulus cracked: %llu\n"
        "HolyC VM opcodes: %zu | HolyC VM cycles: %zu\n"
        "TempleOS GodWords: %s\n"
        "A* Maze nodes expanded: %zu\n"
        "Brainfuck chars: %zu\n"
        "SUBLEQ OISC cycles: %zu\n"
        "GEXE bytes: %zu\n"
        "TCP/IP packets: %zu\n"
        "SHA-256 PoW hashes: %llu (%s)\n"
        "GovnoCPU VM cycles: %llu\n",
        cpu_brand.c_str(),
        result.c_str(),
        static_cast<long long>(holyc_aot_shift),
        poly_langs,
        poly_bytes,
        poly_steps,
        poly_summary.c_str(),
        kernel_mods,
        ml_epochs,
        ml_loss,
        ga_gens,
        static_cast<unsigned long long>(rsa_n),
        hc_ops,
        hc_cyc,
        god_words.c_str(),
        astar_nodes,
        bf_len,
        subleq_cyc,
        gexe_len,
        tcp_pkts,
        static_cast<unsigned long long>(pow_hashes),
        pow_hash.c_str(),
        static_cast<unsigned long long>(vm_cycles)
    );
    std::fflush(stdout);

    return (result == "Hello world") ? 0 : 2;
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int nCmdShow) {
    if (lpCmdLine && std::wcsstr(lpCmdLine, L"--self-test") != nullptr) {
        return run_headless_self_test();
    }
    return govno::gui::run_cursed_gui_app(hInstance, nCmdShow);
}
