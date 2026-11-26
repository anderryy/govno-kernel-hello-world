#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace govno::madness {

struct ApocalypseReport {
    std::string verified_output;
    std::string dump;
    uint64_t aes_keys_tested = 0;
    uint64_t total_rays_traced = 0;
    size_t bigint_digits = 0;
    double best_tsp_length = 0.0;
    double psnr_db = 0.0;
    double pi_estimate = 0.0;
    bool ok = false;
};

ApocalypseReport run_apocalypse_engines(
    const std::string& input,
    bool heavy_mode,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb
);

} // namespace govno::madness
