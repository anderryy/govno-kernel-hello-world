#pragma once

#include <string>

namespace govno::holyc_aot {

struct HolyCAotResult {
    std::string emitted_text;
    std::string aot_log;
    int64_t magic_power = 0;
    int64_t check_val = 0;
};

// Implemented in build/generated_from_holyc.cpp, generated at build-time by holyc2cpp.exe from holyc/hello_temple.HC!
HolyCAotResult run_compiled_holyc_aot();
const char* get_holyc_transpiled_cpp_preview();

} // namespace govno::holyc_aot
