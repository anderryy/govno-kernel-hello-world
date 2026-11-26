#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace govno::madness {

std::array<uint8_t, 32> sha256_bytes(const uint8_t* data, size_t len);
std::string sha256_hex(const std::string& input);

struct ExtremeCSResult {
    std::string output;
    std::string dump;
    bool ok = false;
};

struct HeavyGauntletResult {
    std::string output;
    std::string report;
    uint64_t total_flops_est = 0;
    size_t nqueens_solutions = 0;
    bool ok = false;
};

ExtremeCSResult dft_idft_roundtrip(const std::string& input);
ExtremeCSResult huffman_compress_decompress(const std::string& input);
ExtremeCSResult solve_sudoku_and_nqueens(const std::string& input);
ExtremeCSResult game_of_life_and_graph(const std::string& input);

HeavyGauntletResult run_extreme_compute_gauntlet(
    const std::string& input,
    bool heavy_mode,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb
);

} // namespace govno::madness
