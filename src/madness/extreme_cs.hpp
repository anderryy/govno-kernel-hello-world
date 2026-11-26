#pragma once

#include <array>
#include <cstdint>
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

ExtremeCSResult dft_idft_roundtrip(const std::string& input);
ExtremeCSResult huffman_compress_decompress(const std::string& input);
ExtremeCSResult solve_sudoku_and_nqueens(const std::string& input);
ExtremeCSResult game_of_life_and_graph(const std::string& input);

} // namespace govno::madness
