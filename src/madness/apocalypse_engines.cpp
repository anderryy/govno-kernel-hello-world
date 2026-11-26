#include "apocalypse_engines.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <vector>

namespace govno::madness {
namespace {

// ============================================================================
// 1. FIPS-197 AES-128 + Brute-Force Partial Key Recovery
// ============================================================================
constexpr uint8_t AES_SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

constexpr uint8_t AES_INV_SBOX[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

constexpr uint8_t AES_RCON[11] = { 0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36 };

inline uint8_t xtime(uint8_t x) {
    return static_cast<uint8_t>((x << 1) ^ ((x & 0x80) ? 0x1b : 0x00));
}

inline uint8_t gf_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) p ^= a;
        a = xtime(a);
        b >>= 1;
    }
    return p;
}

void aes128_key_expand(const uint8_t key[16], uint8_t rk[176]) {
    std::memcpy(rk, key, 16);
    int bytes_gen = 16;
    int rcon_idx = 1;
    uint8_t tmp[4];
    while (bytes_gen < 176) {
        std::memcpy(tmp, &rk[bytes_gen - 4], 4);
        if (bytes_gen % 16 == 0) {
            uint8_t t = tmp[0];
            tmp[0] = AES_SBOX[tmp[1]] ^ AES_RCON[rcon_idx++];
            tmp[1] = AES_SBOX[tmp[2]];
            tmp[2] = AES_SBOX[tmp[3]];
            tmp[3] = AES_SBOX[t];
        }
        for (int i = 0; i < 4; ++i) {
            rk[bytes_gen] = rk[bytes_gen - 16] ^ tmp[i];
            ++bytes_gen;
        }
    }
}

void aes128_encrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t rk[176]) {
    uint8_t s[16];
    std::memcpy(s, in, 16);
    for (int i = 0; i < 16; ++i) s[i] ^= rk[i];

    for (int round = 1; round <= 10; ++round) {
        for (int i = 0; i < 16; ++i) s[i] = AES_SBOX[s[i]];
        uint8_t t = s[1]; s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = t;
        std::swap(s[2], s[10]); std::swap(s[6], s[14]);
        t = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = s[3]; s[3] = t;
        if (round < 10) {
            for (int c = 0; c < 4; ++c) {
                int i = c * 4;
                uint8_t a0 = s[i], a1 = s[i+1], a2 = s[i+2], a3 = s[i+3];
                s[i]   = xtime(a0) ^ (xtime(a1) ^ a1) ^ a2 ^ a3;
                s[i+1] = a0 ^ xtime(a1) ^ (xtime(a2) ^ a2) ^ a3;
                s[i+2] = a0 ^ a1 ^ xtime(a2) ^ (xtime(a3) ^ a3);
                s[i+3] = (xtime(a0) ^ a0) ^ a1 ^ a2 ^ xtime(a3);
            }
        }
        const uint8_t* kr = &rk[round * 16];
        for (int i = 0; i < 16; ++i) s[i] ^= kr[i];
    }
    std::memcpy(out, s, 16);
}

void aes128_decrypt_block(const uint8_t in[16], uint8_t out[16], const uint8_t rk[176]) {
    uint8_t s[16];
    std::memcpy(s, in, 16);
    for (int i = 0; i < 16; ++i) s[i] ^= rk[160 + i];

    for (int round = 9; round >= 0; --round) {
        uint8_t t = s[13]; s[13] = s[9]; s[9] = s[5]; s[5] = s[1]; s[1] = t;
        std::swap(s[2], s[10]); std::swap(s[6], s[14]);
        t = s[3]; s[3] = s[7]; s[7] = s[11]; s[11] = s[15]; s[15] = t;
        for (int i = 0; i < 16; ++i) s[i] = AES_INV_SBOX[s[i]];
        const uint8_t* kr = &rk[round * 16];
        for (int i = 0; i < 16; ++i) s[i] ^= kr[i];
        if (round > 0) {
            for (int c = 0; c < 4; ++c) {
                int i = c * 4;
                uint8_t a0 = s[i], a1 = s[i+1], a2 = s[i+2], a3 = s[i+3];
                s[i]   = gf_mul(a0,14) ^ gf_mul(a1,11) ^ gf_mul(a2,13) ^ gf_mul(a3,9);
                s[i+1] = gf_mul(a0,9)  ^ gf_mul(a1,14) ^ gf_mul(a2,11) ^ gf_mul(a3,13);
                s[i+2] = gf_mul(a0,13) ^ gf_mul(a1,9)  ^ gf_mul(a2,14) ^ gf_mul(a3,11);
                s[i+3] = gf_mul(a0,11) ^ gf_mul(a1,13) ^ gf_mul(a2,9)  ^ gf_mul(a3,14);
            }
        }
    }
    std::memcpy(out, s, 16);
}

// ============================================================================
// 2. Arbitrary-Precision BigInt Factorial (base 10^9)
// ============================================================================
size_t compute_bigint_factorial_digits(int n, uint32_t& out_checksum) {
    constexpr uint32_t BASE = 1000000000U;
    std::vector<uint32_t> limbs(1, 1);
    for (int k = 2; k <= n; ++k) {
        uint64_t carry = 0;
        for (size_t i = 0; i < limbs.size(); ++i) {
            uint64_t cur = static_cast<uint64_t>(limbs[i]) * static_cast<uint64_t>(k) + carry;
            limbs[i] = static_cast<uint32_t>(cur % BASE);
            carry = cur / BASE;
        }
        while (carry > 0) {
            limbs.push_back(static_cast<uint32_t>(carry % BASE));
            carry /= BASE;
        }
    }
    size_t digits = (limbs.size() - 1) * 9;
    uint32_t top = limbs.back();
    while (top > 0) { ++digits; top /= 10; }
    out_checksum = 0;
    for (uint32_t v : limbs) out_checksum ^= v;
    return digits;
}

struct Vec3d {
    double x, y, z;
    Vec3d operator+(const Vec3d& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3d operator-(const Vec3d& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3d operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3d operator*(const Vec3d& o) const { return {x * o.x, y * o.y, z * o.z}; }
};
inline double dot_v(const Vec3d& a, const Vec3d& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3d norm_v(const Vec3d& a) {
    double l = std::sqrt(dot_v(a, a));
    return (l > 1e-12) ? (a * (1.0 / l)) : Vec3d{0.0, 1.0, 0.0};
}

} // namespace

ApocalypseReport run_apocalypse_engines(
    const std::string& input,
    bool heavy_mode,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb
) {
    ApocalypseReport rep;
    if (input.size() != 11) return rep;

    std::ostringstream log;
    log << "=== GOVNO-APOCALYPSE HARDWARE BURNER ("
        << (heavy_mode ? "HEAVY FULL" : "FAST VERIFY") << ") ===\r\n";

    // ------------------------------------------------------------------------
    // Stage A: AES-128 Key Recovery Brute Force
    // ------------------------------------------------------------------------
    uint8_t secret_key[16] = { 0xDE,0xAD,0xBE,0xEF,0xCA,0xFE,0xBA,0xBE,0x42,0x69,0x13,0x37,0x00,0x67,0xA5,0x7C };
    if (!heavy_mode) {
        secret_key[13] = 0x00;
        secret_key[14] = 0x04;
        secret_key[15] = 0xD2;
    }
    uint8_t plain_blk[16]{};
    std::memcpy(plain_blk, input.data(), 11);
    const uint8_t tag[5] = { 'G','O','V','N','O' };
    std::memcpy(plain_blk + 11, tag, 5);

    uint8_t rk_true[176];
    aes128_key_expand(secret_key, rk_true);
    uint8_t cipher_blk[16];
    aes128_encrypt_block(plain_blk, cipher_blk, rk_true);

    uint32_t max_space = heavy_mode ? (1U << 23) : (1U << 12);
    uint8_t cand_key[16];
    std::memcpy(cand_key, secret_key, 16);
    std::string aes_recovered;

    for (uint32_t k = 0; k < max_space; ++k) {
        if ((k & 0xFFFFU) == 0 && cancel_flag.load()) return rep;
        cand_key[13] = static_cast<uint8_t>((k >> 16) & 0x7FU);
        cand_key[14] = static_cast<uint8_t>((k >> 8) & 0xFFU);
        cand_key[15] = static_cast<uint8_t>(k & 0xFFU);
        uint8_t rk[176], dec[16];
        aes128_key_expand(cand_key, rk);
        aes128_decrypt_block(cipher_blk, dec, rk);
        ++rep.aes_keys_tested;
        if (dec[11] == 'G' && dec[12] == 'O' && dec[13] == 'V' && dec[14] == 'N' && dec[15] == 'O') {
            aes_recovered.assign(reinterpret_cast<char*>(dec), 11);
            break;
        }
        if (heavy_mode && k > 0 && (k % 850000U == 0)) {
            cb(static_cast<int>((k * 20U) / max_space),
               "[AES-128 CRACK] Перебрано " + std::to_string(k) + " ключей AES-128 (10 раундов GF(2^8))...");
        }
    }
    cb(22, "[AES-128 CRACKED] Ключ найден за " + std::to_string(rep.aes_keys_tested) +
           " расшифровок -> \"" + aes_recovered + "\"");

    // ------------------------------------------------------------------------
    // Stage B: Gray-Scott 2D Reaction-Diffusion PDE Solver
    // ------------------------------------------------------------------------
    const int pde_n = heavy_mode ? 256 : 48;
    const int pde_steps = heavy_mode ? 3400 : 60;
    std::vector<double> U(static_cast<size_t>(pde_n * pde_n), 1.0);
    std::vector<double> V(static_cast<size_t>(pde_n * pde_n), 0.0);
    std::vector<double> U_nxt = U, V_nxt = V;
    int mid = pde_n / 2;
    for (int y = mid - 6; y <= mid + 6; ++y) {
        for (int x = mid - 6; x <= mid + 6; ++x) {
            U[static_cast<size_t>(y * pde_n + x)] = 0.50;
            V[static_cast<size_t>(y * pde_n + x)] = 0.25;
        }
    }
    constexpr double Du = 0.16, Dv = 0.08, F = 0.037, K_gs = 0.060;
    double chem_energy = 0.0;
    for (int st = 0; st < pde_steps; ++st) {
        if ((st & 31) == 0 && cancel_flag.load()) return rep;
        chem_energy = 0.0;
        for (int y = 0; y < pde_n; ++y) {
            int ym = (y == 0) ? (pde_n - 1) : (y - 1);
            int yp = (y + 1 == pde_n) ? 0 : (y + 1);
            for (int x = 0; x < pde_n; ++x) {
                int xm = (x == 0) ? (pde_n - 1) : (x - 1);
                int xp = (x + 1 == pde_n) ? 0 : (x + 1);
                size_t idx = static_cast<size_t>(y * pde_n + x);
                double u = U[idx], v = V[idx];
                double lap_u = U[ym * pde_n + x] + U[yp * pde_n + x] + U[y * pde_n + xm] + U[y * pde_n + xp] - 4.0 * u;
                double lap_v = V[ym * pde_n + x] + V[yp * pde_n + x] + V[y * pde_n + xm] + V[y * pde_n + xp] - 4.0 * v;
                double uvv = u * v * v;
                double un = u + (Du * lap_u - uvv + F * (1.0 - u));
                double vn = v + (Dv * lap_v + uvv - (F + K_gs) * v);
                U_nxt[idx] = std::clamp(un, 0.0, 1.0);
                V_nxt[idx] = std::clamp(vn, 0.0, 1.0);
                chem_energy += V_nxt[idx];
            }
        }
        U.swap(U_nxt);
        V.swap(V_nxt);
    }
    cb(42, "[GRAY-SCOTT PDE] Решены 2D-уравнения реакции-диффузии " + std::to_string(pde_n) + "x" +
           std::to_string(pde_n) + " (" + std::to_string(pde_steps) + " шагов, масса V=" + std::to_string(chem_energy) + ")");

    // ------------------------------------------------------------------------
    // Stage C: 2048x2048 8x8 JPEG 2D-DCT -> Quantize -> IDCT Codec
    // ------------------------------------------------------------------------
    const int img_dim = heavy_mode ? 2048 : 128;
    const int blk_side = img_dim / 8;
    std::array<std::array<double, 8>, 8> cos_lut{};
    constexpr double PI = 3.14159265358979323846;
    for (int k = 0; k < 8; ++k) {
        double alpha = (k == 0) ? (1.0 / std::sqrt(8.0)) : 0.5;
        for (int n = 0; n < 8; ++n) {
            cos_lut[k][n] = alpha * std::cos((PI * (2.0 * n + 1.0) * k) / 16.0);
        }
    }
    double sq_err = 0.0;
    for (int by = 0; by < blk_side; ++by) {
        if ((by & 7) == 0 && cancel_flag.load()) return rep;
        for (int bx = 0; bx < blk_side; ++bx) {
            double blk[8][8], tmp[8][8], dct[8][8], rec[8][8];
            for (int r = 0; r < 8; ++r) {
                for (int c = 0; c < 8; ++c) {
                    int y = by * 8 + r, x = bx * 8 + c;
                    blk[r][c] = 128.0 + 90.0 * std::sin(x * 0.07 + y * 0.05) * std::cos(x * 0.03 - y * 0.04);
                }
            }
            for (int r = 0; r < 8; ++r) {
                for (int u = 0; u < 8; ++u) {
                    double s = 0.0;
                    for (int c = 0; c < 8; ++c) s += blk[r][c] * cos_lut[u][c];
                    tmp[r][u] = s;
                }
            }
            for (int v = 0; v < 8; ++v) {
                for (int u = 0; u < 8; ++u) {
                    double s = 0.0;
                    for (int r = 0; r < 8; ++r) s += tmp[r][u] * cos_lut[v][r];
                    double q = std::round(s / 4.0) * 4.0;
                    dct[v][u] = q;
                }
            }
            for (int v = 0; v < 8; ++v) {
                for (int c = 0; c < 8; ++c) {
                    double s = 0.0;
                    for (int u = 0; u < 8; ++u) s += dct[v][u] * cos_lut[u][c];
                    tmp[v][c] = s;
                }
            }
            for (int r = 0; r < 8; ++r) {
                for (int c = 0; c < 8; ++c) {
                    double s = 0.0;
                    for (int v = 0; v < 8; ++v) s += tmp[v][c] * cos_lut[v][r];
                    rec[r][c] = s;
                    double d = rec[r][c] - blk[r][c];
                    sq_err += d * d;
                }
            }
        }
    }
    double mse_dct = sq_err / static_cast<double>(img_dim * img_dim);
    rep.psnr_db = 10.0 * std::log10((255.0 * 255.0) / std::max(1e-9, mse_dct));
    cb(60, "[JPEG 2D-DCT/IDCT] Обработано " + std::to_string(blk_side * blk_side) + " блоков 8x8 (" +
           std::to_string(img_dim) + "x" + std::to_string(img_dim) + ", PSNR=" + std::to_string(rep.psnr_db) + " dB)");

    // ------------------------------------------------------------------------
    // Stage D: 256-City Simulated Annealing TSP + 25M Monte-Carlo Pi + BigInt N!
    // ------------------------------------------------------------------------
    const int n_cities = heavy_mode ? 256 : 32;
    const int sa_iters = heavy_mode ? 1600000 : 8000;
    std::vector<double> cx(n_cities), cy(n_cities);
    for (int i = 0; i < n_cities; ++i) {
        double ang = (2.0 * PI * i) / n_cities;
        cx[i] = 50.0 * std::cos(ang) + 12.0 * std::sin(ang * 3.0);
        cy[i] = 50.0 * std::sin(ang) + 12.0 * std::cos(ang * 5.0);
    }
    std::vector<int> tour(n_cities);
    std::iota(tour.begin(), tour.end(), 0);
    uint64_t rng_s = 0xCAFEBABEDEADBEEFULL;
    auto fast_rng = [&]() -> uint32_t {
        rng_s = rng_s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<uint32_t>(rng_s >> 32);
    };
    auto edge_len = [&](int a, int b) {
        double dx = cx[a] - cx[b], dy = cy[a] - cy[b];
        return std::sqrt(dx * dx + dy * dy);
    };
    double cur_len = 0.0;
    for (int i = 0; i < n_cities; ++i) cur_len += edge_len(tour[i], tour[(i + 1) % n_cities]);
    double temp = 45.0;
    for (int it = 0; it < sa_iters; ++it) {
        if ((it & 0xFFFF) == 0 && cancel_flag.load()) return rep;
        int i = static_cast<int>(fast_rng() % n_cities);
        int j = static_cast<int>(fast_rng() % n_cities);
        if (i == j) continue;
        if (i > j) std::swap(i, j);
        if (i == 0 && j == n_cities - 1) continue;
        int i_prev = (i + n_cities - 1) % n_cities;
        int j_next = (j + 1) % n_cities;
        double before = edge_len(tour[i_prev], tour[i]) + edge_len(tour[j], tour[j_next]);
        double after  = edge_len(tour[i_prev], tour[j]) + edge_len(tour[i], tour[j_next]);
        double delta = after - before;
        if (delta < 0.0 || (static_cast<double>(fast_rng()) / 4294967296.0) < std::exp(-delta / std::max(0.01, temp))) {
            std::reverse(tour.begin() + i, tour.begin() + j + 1);
            cur_len += delta;
        }
        temp *= 0.999985;
    }
    rep.best_tsp_length = cur_len;

    const uint64_t mc_samples = heavy_mode ? 64000000ULL : 200000ULL;
    uint64_t inside = 0;
    for (uint64_t s = 0; s < mc_samples; ++s) {
        if ((s & 0xFFFFFULL) == 0 && cancel_flag.load()) return rep;
        double rx = static_cast<double>(fast_rng() & 0xFFFFFFU) / 16777216.0;
        double ry = static_cast<double>(fast_rng() & 0xFFFFFFU) / 16777216.0;
        if (rx * rx + ry * ry <= 1.0) ++inside;
    }
    rep.pi_estimate = 4.0 * static_cast<double>(inside) / static_cast<double>(mc_samples);

    uint32_t big_chk = 0;
    int fact_n = heavy_mode ? 5000 : 200;
    rep.bigint_digits = compute_bigint_factorial_digits(fact_n, big_chk);
    cb(78, "[TSP+BIGINT+MC] 256-городов TSP (длина=" + std::to_string(rep.best_tsp_length) + "), " +
           std::to_string(fact_n) + "! (" + std::to_string(rep.bigint_digits) + " цифр), MonteCarlo Pi=" +
           std::to_string(rep.pi_estimate));

    // ------------------------------------------------------------------------
    // Stage E: Monte-Carlo Multi-Bounce Path Tracer Verification
    // ------------------------------------------------------------------------
    const int pw = heavy_mode ? 800 : 96;
    const int ph = heavy_mode ? 450 : 54;
    const int spp = heavy_mode ? 20 : 2;
    double total_lum = 0.0;
    for (int y = 0; y < ph; ++y) {
        if ((y & 15) == 0 && cancel_flag.load()) return rep;
        for (int x = 0; x < pw; ++x) {
            Vec3d accum{0.0, 0.0, 0.0};
            for (int s = 0; s < spp; ++s) {
                double jx = static_cast<double>(fast_rng() & 0xFFFFU) / 65536.0;
                double jy = static_cast<double>(fast_rng() & 0xFFFFU) / 65536.0;
                double nx = ((x + jx) / pw - 0.5) * 1.8;
                double ny = (0.5 - (y + jy) / ph) * 1.0;
                Vec3d ro{0.0, 0.2, -3.0}, rd = norm_v({nx, ny, 1.4}), throughput{1.0, 1.0, 1.0};
                for (int bnc = 0; bnc < 3; ++bnc) {
                    ++rep.total_rays_traced;
                    Vec3d oc = ro - Vec3d{0.0, 0.0, 0.0};
                    double b = dot_v(oc, rd), c = dot_v(oc, oc) - 0.85 * 0.85;
                    double disc = b * b - c;
                    if (disc > 0.0 && (-b - std::sqrt(disc)) > 0.001) {
                        double t = -b - std::sqrt(disc);
                        ro = ro + rd * t;
                        Vec3d n = norm_v(ro);
                        rd = norm_v(rd - n * (2.0 * dot_v(rd, n)));
                        throughput = throughput * Vec3d{0.75, 0.85, 0.95};
                    } else {
                        double sky = std::clamp(rd.y * 0.5 + 0.5, 0.0, 1.0);
                        Vec3d sky_col{0.2 + 0.6 * sky, 0.3 + 0.5 * sky, 0.8};
                        accum = accum + throughput * sky_col;
                        break;
                    }
                }
            }
            total_lum += (accum.x + accum.y + accum.z) / static_cast<double>(spp);
        }
    }

    log << "  AES-128 Key Recovery    : " << rep.aes_keys_tested << " decrypts -> \"" << aes_recovered << "\"\r\n"
        << "  Gray-Scott 2D PDE Grid  : " << pde_n << "x" << pde_n << " x " << pde_steps << " steps (mass=" << chem_energy << ")\r\n"
        << "  JPEG 2D-DCT/IDCT Codec  : " << img_dim << "x" << img_dim << " (PSNR=" << std::fixed << std::setprecision(2) << rep.psnr_db << " dB)\r\n"
        << "  256-City Annealed TSP   : length=" << rep.best_tsp_length << " (" << sa_iters << " 2-opt moves)\r\n"
        << "  BigInt " << fact_n << "! Exact    : " << rep.bigint_digits << " decimal digits (chk=" << big_chk << ")\r\n"
        << "  Monte-Carlo Pi          : " << std::setprecision(6) << rep.pi_estimate << " (" << mc_samples << " samples)\r\n"
        << "  Path Tracer GI          : " << pw << "x" << ph << "x" << spp << " spp (" << rep.total_rays_traced << " rays, lum=" << total_lum << ")\r\n";

    cb(100, "[PATH-TRACER GI] Отрейтрейсено " + std::to_string(rep.total_rays_traced) +
            " лучей (" + std::to_string(pw) + "x" + std::to_string(ph) + " x" + std::to_string(spp) + " spp)");

    rep.verified_output = (aes_recovered == input && rep.psnr_db > 25.0 && std::abs(rep.pi_estimate - 3.14159) < 0.02)
                              ? aes_recovered
                              : "APOCALYPSE_FAIL";
    rep.dump = log.str();
    rep.ok = (rep.verified_output == input);
    return rep;
}

} // namespace govno::madness
