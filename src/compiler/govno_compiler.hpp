#pragma once

#include "kernel_types.hpp"
#include <functional>
#include <string>
#include <vector>

namespace govno::compiler {

enum class GovnoOpcode : uint8_t {
    OP_NOP      = 0x00,
    OP_ADD_MEM  = 0x01,
    OP_SUB_MEM  = 0x02,
    OP_PTR_INC  = 0x03,
    OP_PTR_DEC  = 0x04,
    OP_JZ       = 0x05,
    OP_JNZ      = 0x06,
    OP_SYSCALL  = 0x07,
    OP_HALT     = 0xFF
};

uint32_t compute_crc32(const uint8_t* data, size_t len);
std::string format_hex_dump(const std::vector<uint8_t>& bytes);

// Compiles input_payload through:
// XML -> JSON -> SQL -> #DEFINE -> Lexer -> AST -> Deopt IR -> Lisp -> C99 -> Brainfuck -> SUBLEQ OISC -> GovnoASM -> GovnoLD -> .gexe
bool run_full_compiler_pipeline(
    const std::string& input_payload,
    CompilerArtifacts& out_artifacts,
    const std::function<void(int pct, const std::string& log_msg)>& progress_cb,
    const std::function<void(int ms)>& sleep_cb
);

} // namespace govno::compiler
