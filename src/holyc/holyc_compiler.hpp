#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace govno::holyc {

struct HolyCCompileRunResult {
    bool success = true;
    std::string error_msg;
    std::string emitted_output;
    std::string printed_text;
    std::string raw_and_expanded_source;
    std::string vm_and_doldoc_report;
    std::string god_oracle_words;
    size_t opcodes_count = 0;
    size_t vm_cycles = 0;
};

HolyCCompileRunResult compile_and_run_holyc_stage(
    const std::string& payload,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb
);

// Compiles and runs arbitrary HolyC code written by the user in the live GUI IDE!
HolyCCompileRunResult execute_custom_holyc_source(const std::string& user_holyc_src);

} // namespace govno::holyc
