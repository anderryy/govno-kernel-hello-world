#pragma once

#include <atomic>
#include <functional>
#include <string>

namespace govno::kernel {

struct HardwareProbeInfo {
    std::string cpu_brand;
    uint64_t total_ram_mb = 0;
    uint64_t uptime_sec = 0;
};

HardwareProbeInfo probe_real_hardware_for_comedy();

size_t simulate_kernel_source_tree_build(
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& progress_cb,
    const std::function<void(int ms)>& sleep_cb
);

} // namespace govno::kernel
