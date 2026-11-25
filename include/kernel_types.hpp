#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <mutex>

namespace govno {

enum class PipelineStage : int {
    IDLE = 0,
    STAGE_1_TEMPLATE_ORACLE,
    STAGE_2_BIOS_AND_KERNEL_BUILD,
    STAGE_3_NEURAL_NET_TRAINING,
    STAGE_4_GENETIC_EVOLUTION,
    STAGE_5_RSA_CRYPTO_AND_CRACK,
    STAGE_6_LZ77_BASE_MORSE,
    STAGE_7_HOLYC_TEMPLEOS_COMPILER,
    STAGE_8_XML_JSON_SQL,
    STAGE_9_PREPROCESSOR_LEXER_AST,
    STAGE_10_IR_LISP_C_BRAINFUCK,
    STAGE_11_SUBLEQ_OISC,
    STAGE_12_ASM_LINKER,
    STAGE_13_BLOCKCHAIN_AND_TCPIP,
    STAGE_14_DRIVERS_AND_VM_EXEC,
    STAGE_15_GRAND_FINALE_HELLO_WORLD,
    STAGE_ERROR
};

union VGACell {
    uint16_t raw;
    struct {
        uint8_t ch;
        uint8_t attr;
    } parts;
};

struct CpuSnapshot {
    uint64_t rip = 0;
    uint64_t rax = 0;
    uint64_t rbx = 0;
    uint64_t rcx = 0;
    uint64_t rsp = 0xFFFF0000;
    uint64_t tape_ptr = 0;
    uint8_t  tape_val = 0;
    uint64_t cycles = 0;
    uint64_t subleq_cycles = 0;
    uint64_t holyc_cycles = 0;
    uint64_t syscalls = 0;
    uint64_t page_faults = 0;
    uint32_t ram_allocated_kb = 0;
    uint64_t packets_sent = 0;
    uint64_t pow_hashes = 0;
    bool     halted = false;
    std::string current_op = "NOP";
    std::string cpu_brand = "Unknown Potato CPU";
    std::string god_words = "DIVINE BLESSING PENDING";
    std::array<uint8_t, 16> tape_preview{};
};

struct CompilerArtifacts {
    std::string ml_genetics_dump;
    std::string rsa_lz77_morse_dump;
    std::string holyc_aot_dump;
    std::string holyc_source_dump;
    std::string holyc_vm_dump;
    std::string astar_maze_dump;
    std::string xml_json_sql_dump;
    std::string govnoscript_source;
    std::string tokens_dump;
    std::string ast_dump;
    std::string ir_dump;
    std::string lisp_and_c_dump;
    std::string brainfuck_code;
    std::string brainfuck_verified_output;
    std::string subleq_oisc_dump;
    std::string govnoasm_and_linker_dump;
    std::string tcp_packets_dump;
    std::string gexe_and_blockchain_dump;
    std::vector<uint8_t> gexe_bytecode;

    size_t kernel_modules_built = 0;
    size_t ml_epochs = 0;
    float  ml_final_loss = 0.0f;
    size_t genetic_generations = 0;
    uint64_t rsa_modulus = 0;
    int64_t holyc_aot_shift = 0;
    size_t holyc_opcodes_count = 0;
    size_t holyc_vm_cycles = 0;
    size_t astar_nodes_expanded = 0;
    std::string god_oracle_words;
    size_t subleq_cycles = 0;
    size_t sql_rows_scanned = 0;
    size_t token_count = 0;
    size_t ast_node_count = 0;
    size_t ir_instruction_count = 0;
    size_t lisp_forms_count = 0;
    size_t c_lines_count = 0;
    size_t bf_length = 0;
    size_t asm_line_count = 0;
    size_t gexe_size_bytes = 0;
    size_t tcp_packet_count = 0;
    uint64_t pow_nonce = 0;
    std::string pow_block_hash;
};

struct PipelineSharedState {
    std::mutex mtx;
    PipelineStage stage = PipelineStage::IDLE;
    std::string status_banner = "Готов к тотальному технологическому безумию...";
    int kernel_progress = 0;
    int ml_crypto_progress = 0;
    int compiler_progress = 0;
    int driver_net_progress = 0;
    uint64_t elapsed_ms = 0;
    bool turbo_mode = false;
    bool completed = false;
    bool failed = false;

    std::vector<float> ml_loss_history;
    std::string ml_live_prediction;
    std::vector<std::string> kernel_logs;
    std::string final_vga_output;
    std::array<VGACell, 80> vga_line{};
    CpuSnapshot cpu;
    CompilerArtifacts artifacts;
};

inline PipelineSharedState& g_state() {
    static PipelineSharedState instance;
    return instance;
}

inline void append_kernel_log(const std::string& line) {
    auto& st = g_state();
    std::lock_guard<std::mutex> lock(st.mtx);
    st.kernel_logs.push_back(line);
    if (st.kernel_logs.size() > 4500) {
        st.kernel_logs.erase(st.kernel_logs.begin(), st.kernel_logs.begin() + 600);
    }
}

} // namespace govno
