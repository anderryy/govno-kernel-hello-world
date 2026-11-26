#include "extreme_cs.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

namespace govno::madness {
namespace {

inline uint32_t rotr32(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32U - n));
}

constexpr uint32_t K_SHA256[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

struct HuffNode {
    uint8_t ch = 0;
    int freq = 0;
    bool is_leaf = false;
    std::shared_ptr<HuffNode> left;
    std::shared_ptr<HuffNode> right;
};

void build_huff_codes(const std::shared_ptr<HuffNode>& node, const std::string& prefix,
                      std::map<uint8_t, std::string>& codes) {
    if (!node) return;
    if (node->is_leaf) {
        codes[node->ch] = prefix.empty() ? "0" : prefix;
        return;
    }
    build_huff_codes(node->left, prefix + "0", codes);
    build_huff_codes(node->right, prefix + "1", codes);
}

bool solve_sudoku(int grid[9][9]) {
    int best_r = -1, best_c = -1;
    int best_cand_mask = 0, best_count = 10;

    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (grid[r][c] != 0) continue;
            int used = 0;
            for (int i = 0; i < 9; ++i) {
                if (grid[r][i]) used |= (1 << grid[r][i]);
                if (grid[i][c]) used |= (1 << grid[i][c]);
            }
            int br = (r / 3) * 3, bc = (c / 3) * 3;
            for (int dr = 0; dr < 3; ++dr) {
                for (int dc = 0; dc < 3; ++dc) {
                    if (grid[br + dr][bc + dc]) used |= (1 << grid[br + dr][bc + dc]);
                }
            }
            int avail_mask = (~used) & 0x3FE; // bits 1..9
            int cnt = 0;
            for (int d = 1; d <= 9; ++d) {
                if (avail_mask & (1 << d)) ++cnt;
            }
            if (cnt == 0) return false;
            if (cnt < best_count) {
                best_count = cnt;
                best_r = r;
                best_c = c;
                best_cand_mask = avail_mask;
            }
        }
    }

    if (best_r == -1) return true;
    for (int d = 1; d <= 9; ++d) {
        if (best_cand_mask & (1 << d)) {
            grid[best_r][best_c] = d;
            if (solve_sudoku(grid)) return true;
            grid[best_r][best_c] = 0;
        }
    }
    return false;
}

void count_8_queens(int row, int cols, int d1, int d2, int& solutions) {
    if (row == 8) {
        ++solutions;
        return;
    }
    int avail = 0xFF & ~(cols | d1 | d2);
    while (avail) {
        int bit = avail & -avail;
        avail ^= bit;
        count_8_queens(row + 1, cols | bit, (d1 | bit) << 1, (d2 | bit) >> 1, solutions);
    }
}

} // namespace

// ============================================================================
// FIPS-180-4 SHA-256
// ============================================================================
std::array<uint8_t, 32> sha256_bytes(const uint8_t* data, size_t len) {
    uint32_t h[8] = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
    };

    std::vector<uint8_t> msg(data, data + len);
    msg.push_back(0x80U);
    while ((msg.size() % 64U) != 56U) msg.push_back(0x00U);
    uint64_t bit_len = static_cast<uint64_t>(len) * 8ULL;
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xFFULL));
    }

    uint32_t w[64];
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64U) {
        for (int i = 0; i < 16; ++i) {
            size_t o = chunk + static_cast<size_t>(i * 4);
            w[i] = (static_cast<uint32_t>(msg[o]) << 24) |
                   (static_cast<uint32_t>(msg[o + 1]) << 16) |
                   (static_cast<uint32_t>(msg[o + 2]) << 8) |
                   static_cast<uint32_t>(msg[o + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = hh + S1 + ch + K_SHA256[i] + w[i];
            uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
            hh = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    std::array<uint8_t, 32> digest{};
    for (int i = 0; i < 8; ++i) {
        digest[i * 4 + 0] = static_cast<uint8_t>((h[i] >> 24) & 0xFFU);
        digest[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xFFU);
        digest[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xFFU);
        digest[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xFFU);
    }
    return digest;
}

std::string sha256_hex(const std::string& input) {
    auto d = sha256_bytes(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : d) oss << std::setw(2) << static_cast<unsigned>(b);
    return oss.str();
}

// ============================================================================
// Discrete Fourier Transform + Inverse DFT
// ============================================================================
ExtremeCSResult dft_idft_roundtrip(const std::string& input) {
    ExtremeCSResult res;
    if (input.empty()) return res;

    constexpr double TWO_PI = 6.28318530717958647692;
    size_t N = input.size();
    std::vector<std::complex<double>> spectrum(N);

    for (size_t k = 0; k < N; ++k) {
        std::complex<double> sum(0.0, 0.0);
        for (size_t n = 0; n < N; ++n) {
            double angle = -TWO_PI * static_cast<double>(k * n) / static_cast<double>(N);
            double x_n = static_cast<double>(static_cast<unsigned char>(input[n]));
            sum += std::polar(x_n, angle);
        }
        spectrum[k] = sum;
    }

    std::string reconstructed;
    double sq_err = 0.0;
    for (size_t n = 0; n < N; ++n) {
        std::complex<double> sum(0.0, 0.0);
        for (size_t k = 0; k < N; ++k) {
            double angle = TWO_PI * static_cast<double>(k * n) / static_cast<double>(N);
            sum += spectrum[k] * std::polar(1.0, angle);
        }
        double val = sum.real() / static_cast<double>(N);
        double orig = static_cast<double>(static_cast<unsigned char>(input[n]));
        sq_err += (val - orig) * (val - orig);
        reconstructed.push_back(static_cast<char>(std::llround(val) & 0xFF));
    }

    double rms = std::sqrt(sq_err / static_cast<double>(N));
    std::ostringstream os;
    os << "[Fourier Spectral Codec (DFT -> IDFT, N=" << N << ")]\n"
       << "  DC Bin X[0] magnitude : " << std::fixed << std::setprecision(2) << std::abs(spectrum[0])
       << " (sum=" << std::llround(spectrum[0].real()) << ")\n"
       << "  Harmonics Magnitude   :";
    for (size_t k = 0; k < std::min<size_t>(N, 6); ++k) {
        os << " |X[" << k << "]|=" << std::setprecision(1) << std::abs(spectrum[k]);
    }
    os << "\n  IDFT RMS Error        : " << std::scientific << std::setprecision(3) << rms << "\n"
       << "  Reconstructed Signal  : \"" << reconstructed << "\"\n";

    res.output = reconstructed;
    res.dump = os.str();
    res.ok = (reconstructed == input);
    return res;
}

// ============================================================================
// Binary Huffman Tree Bit-Codec
// ============================================================================
ExtremeCSResult huffman_compress_decompress(const std::string& input) {
    ExtremeCSResult res;
    if (input.empty()) return res;

    std::map<uint8_t, int> freq;
    for (unsigned char c : input) freq[c]++;

    auto cmp = [](const std::shared_ptr<HuffNode>& a, const std::shared_ptr<HuffNode>& b) {
        if (a->freq != b->freq) return a->freq > b->freq;
        return a->ch > b->ch;
    };
    std::priority_queue<std::shared_ptr<HuffNode>, std::vector<std::shared_ptr<HuffNode>>, decltype(cmp)> pq(cmp);
    for (const auto& kv : freq) {
        auto n = std::make_shared<HuffNode>();
        n->ch = kv.first;
        n->freq = kv.second;
        n->is_leaf = true;
        pq.push(n);
    }
    while (pq.size() > 1) {
        auto l = pq.top(); pq.pop();
        auto r = pq.top(); pq.pop();
        auto p = std::make_shared<HuffNode>();
        p->freq = l->freq + r->freq;
        p->ch = std::min(l->ch, r->ch);
        p->left = l;
        p->right = r;
        pq.push(p);
    }
    auto root = pq.top();
    std::map<uint8_t, std::string> codes;
    build_huff_codes(root, "", codes);

    std::string bitstream;
    for (unsigned char c : input) bitstream += codes[c];

    std::string decoded;
    auto cur = root;
    for (char bit : bitstream) {
        if (!cur->is_leaf) {
            cur = (bit == '0') ? cur->left : cur->right;
        }
        if (cur && cur->is_leaf) {
            decoded.push_back(static_cast<char>(cur->ch));
            cur = root;
        }
    }

    std::ostringstream os;
    os << "[Binary Huffman Prefix Codec]\n"
       << "  Raw Size    : " << (input.size() * 8) << " bits (" << input.size() << " B)\n"
       << "  Encoded Bits: " << bitstream.size() << " bits (ratio "
       << std::fixed << std::setprecision(1) << (100.0 * static_cast<double>(bitstream.size()) / static_cast<double>(input.size() * 8)) << "%)\n"
       << "  Bitstream   : " << bitstream << "\n"
       << "  Symbol Dict : ";
    for (const auto& kv : codes) {
        os << "'" << static_cast<char>(kv.first) << "'=" << kv.second << " ";
    }
    os << "\n  Decoded Out : \"" << decoded << "\"\n";

    res.output = decoded;
    res.dump = os.str();
    res.ok = (decoded == input);
    return res;
}

// ============================================================================
// 9x9 Sudoku Backtracking + 8-Queens 92-Solution Verifier
// ============================================================================
ExtremeCSResult solve_sudoku_and_nqueens(const std::string& input) {
    ExtremeCSResult res;
    int grid[9][9] = {
        {8,0,0, 0,0,0, 0,0,0},
        {0,0,3, 6,0,0, 0,0,0},
        {0,7,0, 0,9,0, 2,0,0},

        {0,5,0, 0,0,7, 0,0,0},
        {0,0,0, 0,4,5, 7,0,0},
        {0,0,0, 1,0,0, 0,3,0},

        {0,0,1, 0,0,0, 0,6,8},
        {0,0,8, 5,0,0, 0,1,0},
        {0,9,0, 0,0,0, 4,0,0}
    };
    bool sudoku_ok = solve_sudoku(grid);
    int diag_sum = 0;
    for (int i = 0; i < 9; ++i) diag_sum += grid[i][i];

    int queens_solutions = 0;
    count_8_queens(0, 0, 0, 0, queens_solutions);

    // Encode/decode check tied cleanly to verified combinatorial constants (Sudoku row-sum 45 & 92 solutions)
    int row0_sum = 0;
    for (int c = 0; c < 9; ++c) row0_sum += grid[0][c];
    uint8_t invariant_key = static_cast<uint8_t>((row0_sum ^ queens_solutions) & 0xFF);
    std::vector<uint8_t> masked;
    for (unsigned char ch : input) masked.push_back(ch ^ invariant_key);
    std::string restored;
    for (uint8_t b : masked) restored.push_back(static_cast<char>(b ^ invariant_key));

    std::ostringstream os;
    os << "[NP-Complete Combinatorics: Arto Inkala Sudoku + 8-Queens]\n"
       << "  Sudoku Solved     : " << (sudoku_ok ? "YES" : "NO")
       << " (Row0 sum=" << row0_sum << ", Diag sum=" << diag_sum << ")\n"
       << "  8-Queens Solutions: " << queens_solutions << " distinct configurations\n"
       << "  Invariant Key     : 0x" << std::hex << static_cast<unsigned>(invariant_key) << std::dec << "\n"
       << "  Solved Row[0..2]  : ";
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 9; ++c) os << grid[r][c];
        os << " ";
    }
    os << "\n  Verified Payload  : \"" << restored << "\"\n";

    res.output = restored;
    res.dump = os.str();
    res.ok = sudoku_ok && (row0_sum == 45) && (queens_solutions == 92) && (restored == input);
    return res;
}

// ============================================================================
// Conway's Game of Life (32x16 x 40 gens) + Floyd-Warshall / Dijkstra Routing
// ============================================================================
ExtremeCSResult game_of_life_and_graph(const std::string& input) {
    ExtremeCSResult res;
    constexpr int W = 32, H = 16;
    std::vector<uint8_t> cur(W * H, 0), nxt(W * H, 0);

    // Seed R-pentomino + payload parity pattern
    auto set_cell = [&](int x, int y) {
        cur[((y + H) % H) * W + ((x + W) % W)] = 1;
    };
    set_cell(16, 7); set_cell(17, 7); set_cell(15, 8); set_cell(16, 8); set_cell(16, 9);
    for (size_t i = 0; i < input.size(); ++i) {
        if ((static_cast<unsigned char>(input[i]) & 1) != 0) set_cell(static_cast<int>(i) + 2, 2);
    }

    int pop_final = 0;
    for (int gen = 0; gen < 40; ++gen) {
        pop_final = 0;
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int nbrs = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = (x + dx + W) % W;
                        int ny = (y + dy + H) % H;
                        nbrs += cur[ny * W + nx];
                    }
                }
                uint8_t alive = cur[y * W + x];
                uint8_t next_state = (alive && (nbrs == 2 || nbrs == 3)) || (!alive && nbrs == 3) ? 1 : 0;
                nxt[y * W + x] = next_state;
                pop_final += next_state;
            }
        }
        cur.swap(nxt);
    }

    // Floyd-Warshall + shortest path character transport over N+1 nodes
    size_t N = input.size() + 1;
    constexpr int INF = 1'000'000;
    std::vector<std::vector<int>> dist(N, std::vector<int>(N, INF));
    std::vector<std::vector<int>> next_hop(N, std::vector<int>(N, -1));
    std::vector<std::vector<char>> edge_char(N, std::vector<char>(N, 0));

    for (size_t i = 0; i < N; ++i) dist[i][i] = 0;
    for (size_t i = 0; i < input.size(); ++i) {
        int w = 10 + (static_cast<unsigned char>(input[i]) % 7);
        dist[i][i + 1] = w;
        next_hop[i][i + 1] = static_cast<int>(i + 1);
        edge_char[i][i + 1] = input[i];
        if (i + 2 < N) {
            dist[i][i + 2] = 500; // decoy shortcut rejected by shortest weight
            next_hop[i][i + 2] = static_cast<int>(i + 2);
            edge_char[i][i + 2] = '?';
        }
    }
    for (size_t k = 0; k < N; ++k) {
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                if (dist[i][k] < INF && dist[k][j] < INF && dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    next_hop[i][j] = next_hop[i][k];
                }
            }
        }
    }

    std::string routed;
    int u = 0;
    while (u != static_cast<int>(N - 1) && u != -1 && routed.size() <= input.size()) {
        int v = next_hop[u][N - 1];
        if (v < 0) break;
        routed.push_back(edge_char[u][v]);
        u = v;
    }

    std::ostringstream os;
    os << "[Conway Game of Life (32x16, 40 gens) + Floyd-Warshall Router]\n"
       << "  Final Alive Cells : " << pop_final << " (toroidal R-pentomino)\n"
       << "  Shortest Path Cost: " << dist[0][N - 1] << " across " << N << " nodes\n"
       << "  Gol Snapshot Top 4 rows:\n";
    for (int y = 0; y < 4; ++y) {
        os << "    ";
        for (int x = 0; x < W; ++x) os << (cur[y * W + x] ? '#' : '.');
        os << "\n";
    }
    os << "  Routed Payload    : \"" << routed << "\"\n";

    res.output = routed;
    res.dump = os.str();
    res.ok = (routed == input);
    return res;
}

namespace {

void count_n_queens_general(int n, int row, uint32_t cols, uint32_t d1, uint32_t d2, size_t& count) {
    if (row == n) {
        ++count;
        return;
    }
    uint32_t mask = (1U << n) - 1U;
    uint32_t avail = mask & ~(cols | d1 | d2);
    while (avail) {
        uint32_t bit = avail & (~avail + 1U);
        avail ^= bit;
        count_n_queens_general(n, row + 1, cols | bit, (d1 | bit) << 1, (d2 | bit) >> 1, count);
    }
}

} // namespace

HeavyGauntletResult run_extreme_compute_gauntlet(
    const std::string& input,
    bool heavy_mode,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb
) {
    HeavyGauntletResult out;
    if (input.empty()) return out;

    uint64_t flops = 0;
    std::ostringstream rep;
    rep << "\r\n=== EXTREME COMPUTE GAUNTLET (" << (heavy_mode ? "OVERKILL REAL WORKLOAD" : "FAST VERIFY") << ") ===\r\n";

    // 1. Dense Gaussian Elimination (720x720 or 64x64)
    const size_t mat_n = heavy_mode ? 720 : 64;
    std::vector<double> A(mat_n * mat_n, 0.0);
    std::vector<double> b(mat_n, 0.0);
    for (size_t i = 0; i < mat_n; ++i) {
        double row_sum = 0.0;
        for (size_t j = 0; j < mat_n; ++j) {
            double v = std::cos(static_cast<double>((i + 1) * (j + 1)) * 0.137) +
                       0.5 * std::sin(static_cast<double>(i + 3) / static_cast<double>(j + 2));
            A[i * mat_n + j] = v;
            row_sum += std::abs(v);
        }
        A[i * mat_n + i] += row_sum + 8.0; // strictly diagonally dominant
        b[i] = static_cast<double>(static_cast<unsigned char>(input[i % input.size()])) + 0.25 * static_cast<double>(i);
    }
    std::vector<double> A_orig = A;
    std::vector<double> b_orig = b;

    for (size_t k = 0; k < mat_n; ++k) {
        if ((k & 15u) == 0 && cancel_flag.load()) return out;
        size_t piv = k;
        double max_v = std::abs(A[k * mat_n + k]);
        for (size_t i = k + 1; i < mat_n; ++i) {
            double v = std::abs(A[i * mat_n + k]);
            if (v > max_v) { max_v = v; piv = i; }
        }
        if (piv != k) {
            for (size_t j = k; j < mat_n; ++j) std::swap(A[k * mat_n + j], A[piv * mat_n + j]);
            std::swap(b[k], b[piv]);
        }
        double inv_diag = 1.0 / A[k * mat_n + k];
        for (size_t i = k + 1; i < mat_n; ++i) {
            double factor = A[i * mat_n + k] * inv_diag;
            for (size_t j = k; j < mat_n; ++j) {
                A[i * mat_n + j] -= factor * A[k * mat_n + j];
            }
            b[i] -= factor * b[k];
            flops += (mat_n - k) * 2ULL;
        }
    }
    std::vector<double> x(mat_n, 0.0);
    for (int i = static_cast<int>(mat_n) - 1; i >= 0; --i) {
        double sum = b[static_cast<size_t>(i)];
        for (size_t j = static_cast<size_t>(i) + 1; j < mat_n; ++j) {
            sum -= A[static_cast<size_t>(i) * mat_n + j] * x[j];
        }
        x[static_cast<size_t>(i)] = sum / A[static_cast<size_t>(i) * mat_n + static_cast<size_t>(i)];
    }
    double max_resid = 0.0;
    for (size_t i = 0; i < mat_n; ++i) {
        double ax = 0.0;
        for (size_t j = 0; j < mat_n; ++j) ax += A_orig[i * mat_n + j] * x[j];
        max_resid = std::max(max_resid, std::abs(ax - b_orig[i]));
    }
    cb(25, "[MATRIX-LU] Решена плотная СЛАУ " + std::to_string(mat_n) + "x" + std::to_string(mat_n) +
           " (невязка=" + std::to_string(max_resid) + ")");

    // 2. O(N^2 * T) N-Body Gravitational Particle Simulation
    const size_t nbodies = heavy_mode ? 1400 : 96;
    const size_t nsteps = heavy_mode ? 450 : 28;
    std::vector<double> px(nbodies), py(nbodies), pz(nbodies), vx(nbodies, 0.0), vy(nbodies, 0.0), vz(nbodies, 0.0);
    for (size_t i = 0; i < nbodies; ++i) {
        double a = static_cast<double>(i) * 0.61803398875;
        px[i] = std::cos(a) * (1.0 + 0.1 * static_cast<double>(i % 13));
        py[i] = std::sin(a) * (1.0 + 0.1 * static_cast<double>(i % 17));
        pz[i] = std::sin(a * 0.5) * 0.5;
    }
    constexpr double DT = 0.004;
    for (size_t step = 0; step < nsteps; ++step) {
        if ((step & 7u) == 0 && cancel_flag.load()) return out;
        for (size_t i = 0; i < nbodies; ++i) {
            double fx = 0.0, fy = 0.0, fz = 0.0;
            for (size_t j = 0; j < nbodies; ++j) {
                if (i == j) continue;
                double dx = px[j] - px[i];
                double dy = py[j] - py[i];
                double dz = pz[j] - pz[i];
                double d2 = dx * dx + dy * dy + dz * dz + 0.04;
                double inv = 1.0 / (d2 * std::sqrt(d2));
                fx += dx * inv;
                fy += dy * inv;
                fz += dz * inv;
                flops += 14ULL;
            }
            vx[i] += fx * 0.002 * DT;
            vy[i] += fy * 0.002 * DT;
            vz[i] += fz * 0.002 * DT;
        }
        for (size_t i = 0; i < nbodies; ++i) {
            px[i] += vx[i] * DT;
            py[i] += vy[i] * DT;
            pz[i] += vz[i] * DT;
        }
        if (heavy_mode && (step + 1) % 60 == 0) {
            cb(25 + static_cast<int>(((step + 1) * 25) / nsteps),
               "[N-BODY GRAVITY] Шаг " + std::to_string(step + 1) + "/" + std::to_string(nsteps) +
               " (" + std::to_string(nbodies) + " частиц, попарные силы Ньютона)");
        }
    }

    // 3. Combinatorics Batch: 14-Queens (365,596 sols!) + Large Game of Life (256x256 x 750 gens)
    int nq_n = heavy_mode ? 14 : 9;
    size_t nq_sols = 0;
    count_n_queens_general(nq_n, 0, 0, 0, 0, nq_sols);
    out.nqueens_solutions = nq_sols;

    const int gw = heavy_mode ? 256 : 32;
    const int gh = heavy_mode ? 256 : 32;
    const int g_gens = heavy_mode ? 750 : 40;
    std::vector<uint8_t> life_a(static_cast<size_t>(gw * gh), 0);
    std::vector<uint8_t> life_b(static_cast<size_t>(gw * gh), 0);
    for (int y = 0; y < gh; ++y) {
        for (int x_i = 0; x_i < gw; ++x_i) {
            life_a[static_cast<size_t>(y * gw + x_i)] = ((x_i * 7 + y * 13) % 5 == 0) ? 1 : 0;
        }
    }
    int life_alive = 0;
    for (int g = 0; g < g_gens; ++g) {
        if ((g & 15) == 0 && cancel_flag.load()) return out;
        life_alive = 0;
        for (int y = 0; y < gh; ++y) {
            int ym = (y == 0) ? (gh - 1) : (y - 1);
            int yp = (y + 1 == gh) ? 0 : (y + 1);
            for (int x_i = 0; x_i < gw; ++x_i) {
                int xm = (x_i == 0) ? (gw - 1) : (x_i - 1);
                int xp = (x_i + 1 == gw) ? 0 : (x_i + 1);
                int nbr = life_a[ym * gw + xm] + life_a[ym * gw + x_i] + life_a[ym * gw + xp] +
                          life_a[y * gw + xm]  +                         life_a[y * gw + xp]  +
                          life_a[yp * gw + xm] + life_a[yp * gw + x_i] + life_a[yp * gw + xp];
                uint8_t st = (life_a[y * gw + x_i] ? (nbr == 2 || nbr == 3) : (nbr == 3)) ? 1 : 0;
                life_b[y * gw + x_i] = st;
                life_alive += st;
            }
        }
        life_a.swap(life_b);
        flops += static_cast<uint64_t>(gw * gh * 10);
    }
    cb(75, "[NP + LIFE] " + std::to_string(nq_n) + "-Ферзей: " + std::to_string(nq_sols) +
           " решений | GameOfLife " + std::to_string(gw) + "x" + std::to_string(gh) + " x" +
           std::to_string(g_gens) + " (живых=" + std::to_string(life_alive) + ")");

    // 4. Deep Complex 4K UHD Mandelbrot Matrix Verification (3840x2160 x 600 iters)
    const int mw = heavy_mode ? 3840 : 192;
    const int mh = heavy_mode ? 2160 : 108;
    const int m_iter = heavy_mode ? 600 : 48;
    uint64_t fractal_iter_sum = 0;
    for (int y = 0; y < mh; ++y) {
        if ((y & 31) == 0 && cancel_flag.load()) return out;
        double ci = -1.2 + 2.4 * static_cast<double>(y) / static_cast<double>(mh);
        for (int x_i = 0; x_i < mw; ++x_i) {
            double cr = -2.0 + 2.8 * static_cast<double>(x_i) / static_cast<double>(mw);
            double zr = 0.0, zi = 0.0;
            int it = 0;
            while (zr * zr + zi * zi <= 4.0 && it < m_iter) {
                double nr = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = nr;
                ++it;
            }
            fractal_iter_sum += static_cast<uint64_t>(it);
            flops += static_cast<uint64_t>(it * 6 + 2);
        }
    }

    rep << "  Linear System (" << mat_n << "x" << mat_n << ") max residual : " << std::scientific << max_resid << "\r\n"
        << "  N-Body Gravity (" << nbodies << " stars, " << nsteps << " steps): COMPLETED\r\n"
        << "  " << nq_n << "-Queens exact solutions      : " << nq_sols << "\r\n"
        << "  Game of Life (" << gw << "x" << gh << ", " << g_gens << " gens) : " << life_alive << " alive\r\n"
        << "  Mandelbrot (" << mw << "x" << mh << ", max=" << m_iter << ") iter_sum: " << std::fixed << fractal_iter_sum << "\r\n"
        << "  Verified Output             : \"" << input << "\"\r\n";

    cb(100, "[MANDELBROT-HDR] Отрендерено " + std::to_string(mw) + "x" + std::to_string(mh) +
            " (iter_sum=" + std::to_string(fractal_iter_sum) + ", ~" + std::to_string(flops / 1000000ULL) + " MFLOPs)");

    out.output = (max_resid < 1e-5 && nq_sols > 0 && fractal_iter_sum > 0) ? input : "GAUNTLET_FAIL";
    out.report = rep.str();
    out.total_flops_est = flops;
    out.ok = (out.output == input);
    return out;
}

} // namespace govno::madness
