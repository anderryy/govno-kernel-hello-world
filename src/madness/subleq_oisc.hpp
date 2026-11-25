#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace govno::madness {

struct SubleqRunResult {
    std::string output;
    size_t instructions_compiled = 0;
    size_t cycles_executed = 0;
    std::string disassembly_dump;
};

SubleqRunResult compile_and_execute_subleq_oisc(const std::vector<uint8_t>& bytes);

} // namespace govno::madness
