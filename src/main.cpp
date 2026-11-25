#include "gui/cursed_gui.hpp"
#include "kernel/govno_kernel.hpp"
#include "kernel_types.hpp"
#include "cursed_macros.hpp"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>

namespace {

int run_headless_self_test() {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* f_out = nullptr;
        freopen_s(&f_out, "CONOUT$", "w", stdout);
        SetConsoleOutputCP(CP_UTF8);
    }

    std::atomic<bool> cancel{false};
    govno::kernel::run_entire_kernel_and_vm_pipeline(cancel, true);

    std::string result, cpu_brand, god_words;
    size_t kernel_mods = 0, ml_epochs = 0, ga_gens = 0, hc_ops = 0, hc_cyc = 0;
    size_t astar_nodes = 0, subleq_cyc = 0, bf_len = 0, gexe_len = 0, tcp_pkts = 0;
    int64_t holyc_aot_shift = 0;
    float ml_loss = 0.0f;
    uint64_t rsa_n = 0, pow_hashes = 0, vm_cycles = 0;

    {
        std::lock_guard<std::mutex> lock(govno::g_state().mtx);
        const auto& st = govno::g_state();
        result = st.final_vga_output;
        cpu_brand = st.cpu.cpu_brand;
        god_words = st.artifacts.god_oracle_words;
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
    }

    std::printf(
        "GOVNO-KERNEL TEMPLE-OS v5.0 (HOLYC AOT + RAYTRACER + A*) SELF-TEST\n"
        "CPU: %s\n"
        "Result: \"%s\"\n"
        "HolyC AOT HOLY_SHIFT (2`3): %lld\n"
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
        "Blockchain PoW hashes: %llu\n"
        "GovnoCPU VM cycles: %llu\n",
        cpu_brand.c_str(),
        result.c_str(),
        static_cast<long long>(holyc_aot_shift),
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
