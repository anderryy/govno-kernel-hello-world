#include "madness/ml_genetics_crypto.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>
#include <unordered_map>

namespace govno::madness {

// ============================================================================
//  1. REAL BACKPROPAGATION MLP NEURAL NETWORK (SGD)
// ============================================================================
NeuralTrainResult train_hello_world_neural_net(
    const std::string& training_target,
    bool heavy_mode,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, float loss, const std::string& cur_pred, const std::string& log_msg)>& cb
) {
    const size_t N = training_target.size();
    constexpr size_t IN_DIM = 11;
    constexpr size_t HID_DIM = 48;
    constexpr float LR = 0.032f;

    auto make_features = [N](size_t idx) {
        std::array<float, IN_DIM> x{};
        constexpr double PI = 3.14159265358979323846;
        double t = (2.0 * PI * static_cast<double>(idx)) / static_cast<double>(std::max<size_t>(1, N));
        for (size_t k = 0; k < 5; ++k) {
            x[k * 2] = static_cast<float>(std::sin(t * static_cast<double>(k + 1)));
            x[k * 2 + 1] = static_cast<float>(std::cos(t * static_cast<double>(k + 1)));
        }
        x[10] = 1.0f;
        return x;
    };

    NeuralTrainResult res;
    std::ostringstream rep;
    const size_t ensemble_models = heavy_mode ? 40 : 1;
    const size_t epochs_per_model = heavy_mode ? 3600 : 400;
    rep << "=== GOVNO-GPT DEEP ENSEMBLE BACKPROP (" << ensemble_models << "x 11 -> " << HID_DIM << " tanh -> 1) ===\r\n";

    std::string cur_pred(N, '?');
    float mse = 1.0f;
    size_t total_epochs = 0;

    for (size_t model_idx = 0; model_idx < ensemble_models; ++model_idx) {
        if (cancel_flag.load()) break;

        std::mt19937 rng(static_cast<uint32_t>(133742 + model_idx * 7919));
        std::uniform_real_distribution<float> w_dist(-0.42f, 0.42f);

        std::array<std::array<float, IN_DIM>, HID_DIM> W1{};
        std::array<float, HID_DIM> b1{};
        std::array<float, HID_DIM> W2{};
        float b2 = 0.5f;

        for (size_t h = 0; h < HID_DIM; ++h) {
            b1[h] = w_dist(rng) * 0.1f;
            W2[h] = w_dist(rng);
            for (size_t i = 0; i < IN_DIM; ++i) {
                W1[h][i] = w_dist(rng);
            }
        }

        for (size_t epoch = 0; epoch < epochs_per_model; ++epoch) {
            if (cancel_flag.load()) break;
            float total_sq_err = 0.0f;

            for (size_t s = 0; s < N; ++s) {
                auto x = make_features(s);
                float target = static_cast<float>(static_cast<uint8_t>(training_target[s])) / 128.0f;

                std::array<float, HID_DIM> h_act{};
                float y_hat = b2;
                for (size_t h = 0; h < HID_DIM; ++h) {
                    float z = b1[h];
                    for (size_t i = 0; i < IN_DIM; ++i) z += W1[h][i] * x[i];
                    h_act[h] = std::tanh(z);
                    y_hat += W2[h] * h_act[h];
                }

                float err = y_hat - target;
                total_sq_err += err * err;

                int pred_ch = std::clamp(static_cast<int>(std::lround(y_hat * 128.0f)), 32, 126);
                cur_pred[s] = static_cast<char>(pred_ch);

                float d_out = err;
                b2 -= LR * d_out;
                for (size_t h = 0; h < HID_DIM; ++h) {
                    float w2_old = W2[h];
                    W2[h] -= LR * d_out * h_act[h];
                    float d_h = d_out * w2_old * (1.0f - h_act[h] * h_act[h]);
                    b1[h] -= LR * d_h;
                    for (size_t i = 0; i < IN_DIM; ++i) {
                        W1[h][i] -= LR * d_h * x[i];
                    }
                }
            }

            ++total_epochs;
            mse = total_sq_err / static_cast<float>(std::max<size_t>(1, N));
            res.epochs_run = total_epochs;
            res.final_mse = mse;

            if (model_idx == 0 && (epoch % 25 == 0 || cur_pred == training_target)) {
                res.loss_history.push_back(mse);
            } else if (heavy_mode && epoch % 400 == 0) {
                res.loss_history.push_back(mse);
            }

            if (total_epochs % (heavy_mode ? 1500 : 120) == 0 ||
                (!heavy_mode && cur_pred == training_target && mse < 1e-5f && epoch >= 220)) {
                size_t denom = ensemble_models * epochs_per_model;
                int pct = static_cast<int>(std::min<size_t>(99, (total_epochs * 100) / std::max<size_t>(1, denom)));
                if (!heavy_mode && cur_pred == training_target && mse < 1e-5f) pct = 100;
                std::ostringstream line;
                line << "[NEURAL-SGD M" << (model_idx + 1) << "/" << ensemble_models << "] Epoch "
                     << std::setw(5) << total_epochs
                     << " | MSE=" << std::fixed << std::setprecision(6) << mse
                     << " | Выход: \"" << cur_pred << "\"";
                rep << line.str() << "\r\n";
                cb(pct, mse, cur_pred, line.str());
            }

            if (!heavy_mode && cur_pred == training_target && mse < 1e-5f && epoch >= 220) {
                break;
            }
        }
    }

    cb(100, mse, cur_pred, "[NEURAL-SGD] Ансамбль обучен (" + std::to_string(res.epochs_run) + " эпох) -> \"" + cur_pred + "\"");
    res.predicted_string = cur_pred;
    rep << "Converged ensemble after " << res.epochs_run << " total epochs -> \"" << res.predicted_string << "\"\r\n";
    res.report = rep.str();
    return res;
}

// ============================================================================
//  2. REAL GENETIC ALGORITHM EVOLUTION (DARWIN SELECTION + CROSSOVER + MUTATION)
// ============================================================================
GeneticEvolveResult evolve_hello_world_genetics(
    const std::string& target,
    bool heavy_mode,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb
) {
    const size_t len = target.size();
    const size_t POP = heavy_mode ? 896 : 96;
    const size_t island_rounds = heavy_mode ? 24 : 1;
    std::mt19937 rng(998877);
    std::uniform_int_distribution<int> char_dist(32, 122);
    std::uniform_int_distribution<size_t> idx_dist(0, POP - 1);
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);

    auto fitness = [&](const std::string& s) -> int {
        int score = 0;
        for (size_t i = 0; i < len; ++i) {
            int diff = std::abs(static_cast<int>(s[i]) - static_cast<int>(target[i]));
            score -= diff;
            if (diff == 0) score += 25;
        }
        return score;
    };

    GeneticEvolveResult res;
    std::ostringstream rep;
    rep << "\r\n=== DARWIN MULTI-ISLAND GENETIC EVOLUTION (POP=" << POP << ", islands=" << island_rounds << ") ===\r\n";

    std::string best(len, ' ');
    size_t total_gens = 0;

    for (size_t island = 0; island < island_rounds; ++island) {
        if (cancel_flag.load()) break;
        std::vector<std::string> pop(POP, std::string(len, ' '));
        for (auto& ind : pop) {
            for (size_t i = 0; i < len; ++i) ind[i] = static_cast<char>(char_dist(rng));
        }

        for (size_t gen = 1; gen <= 400; ++gen) {
            if (cancel_flag.load()) break;
            std::sort(pop.begin(), pop.end(), [&](const std::string& a, const std::string& b) {
                return fitness(a) > fitness(b);
            });
            best = pop[0];
            ++total_gens;
            res.generations = total_gens;

            if (best == target || (gen % 15 == 0)) {
                int pct = static_cast<int>(std::min<size_t>(100, ((island * 40 + gen) * 100) / (island_rounds * 40)));
                if (island == island_rounds - 1 && best == target) pct = 100;
                std::string msg = "[DARWIN-GA #" + std::to_string(island + 1) + "] Gen " + std::to_string(total_gens) +
                                  " (pop=" + std::to_string(POP) + ") | Мутант: \"" + best + "\"";
                rep << msg << "\r\n";
                cb(pct, msg);
            }
            if (best == target) break;

            std::vector<std::string> next_pop;
            next_pop.reserve(POP);
            for (size_t e = 0; e < 8; ++e) next_pop.push_back(pop[e]);

            while (next_pop.size() < POP) {
                auto pick_parent = [&]() -> const std::string& {
                    size_t best_i = idx_dist(rng);
                    for (int t = 0; t < 4; ++t) {
                        size_t cand = idx_dist(rng);
                        if (fitness(pop[cand]) > fitness(pop[best_i])) best_i = cand;
                    }
                    return pop[best_i];
                };
                const auto& p1 = pick_parent();
                const auto& p2 = pick_parent();
                size_t cut = rng() % len;
                std::string child = p1.substr(0, cut) + p2.substr(cut);
                for (size_t i = 0; i < len; ++i) {
                    if (prob_dist(rng) < 0.16f) {
                        int delta = static_cast<int>((rng() % 17)) - 8;
                        int nv = std::clamp(static_cast<int>(child[i]) + delta, 32, 126);
                        child[i] = static_cast<char>(nv);
                    }
                }
                next_pop.push_back(std::move(child));
            }
            pop = std::move(next_pop);
        }
    }

    res.evolved_string = best;
    res.report = rep.str();
    return res;
}

// ============================================================================
//  3. REAL RSA ENCRYPTION + BRUTE-FORCE PRIME FACTORIZATION CRACKER
// ============================================================================
namespace {

uint64_t mul_mod_u64(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t res = 0;
    a %= mod;
    while (b > 0) {
        if (b & 1ULL) {
            res = (res >= mod - a) ? (res - (mod - a)) : (res + a);
        }
        b >>= 1ULL;
        if (b > 0) {
            a = (a >= mod - a) ? (a - (mod - a)) : (a + a);
        }
    }
    return res;
}

uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1ULL) res = mul_mod_u64(res, base, mod);
        base = mul_mod_u64(base, base, mod);
        exp >>= 1ULL;
    }
    return res;
}

int64_t mod_inverse(int64_t a, int64_t m) {
    int64_t m0 = m, x0 = 0, x1 = 1;
    while (a > 1) {
        int64_t q = a / m;
        int64_t t = m;
        m = a % m;
        a = t;
        t = x0;
        x0 = x1 - q * x0;
        x1 = t;
    }
    if (x1 < 0) x1 += m0;
    return x1;
}

bool is_prime_u64(uint64_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (uint64_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

uint64_t next_prime_after(uint64_t start) {
    uint64_t c = (start | 1ULL);
    while (!is_prime_u64(c)) c += 2;
    return c;
}

} // namespace

RsaCrackResult rsa_encrypt_and_crack(
    const std::string& input,
    bool heavy_mode,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb
) {
    const size_t rounds = heavy_mode ? 18 : 1;
    const uint64_t base_p = heavy_mode ? 28000000ULL : 257ULL;
    constexpr uint64_t E = 65537ULL;

    std::string decrypted = input;
    uint64_t last_n = 0, last_p = 0, last_q = 0, last_d = 0;
    std::ostringstream rep;
    rep << "=== REAL RSA ENCRYPTION + MULTI-KEY BRUTE-FORCE FACTORIZATION (" << rounds << " keys) ===\r\n";

    for (size_t r = 0; r < rounds; ++r) {
        if (cancel_flag.load()) break;
        uint64_t p_secret = next_prime_after(base_p + r * 31337ULL);
        uint64_t q_secret = next_prime_after(p_secret + 1009ULL);
        uint64_t n = p_secret * q_secret;
        uint64_t e_use = (n > 100000ULL) ? E : 17ULL;

        std::vector<uint64_t> cipher;
        cipher.reserve(decrypted.size());
        for (unsigned char ch : decrypted) {
            cipher.push_back(mod_pow(ch, e_use, n));
        }

        if (r == 0) {
            cb(20, "[RSA-ENCRYPT] Зашифровано ключом N=" + std::to_string(n) + ", e=" + std::to_string(e_use) +
                   " (прямой перебор делителей N...)");
        }

        uint64_t found_p = 0, found_q = 0;
        if (n % 2 == 0) { found_p = 2; found_q = n / 2; }
        else {
            for (uint64_t cand = 3; cand * cand <= n; cand += 2) {
                if ((cand & 0xFFFFULL) == 1 && cancel_flag.load()) break;
                if (n % cand == 0) {
                    found_p = cand;
                    found_q = n / cand;
                    break;
                }
            }
        }
        if (found_p == 0) break;

        uint64_t phi = (found_p - 1) * (found_q - 1);
        uint64_t d = static_cast<uint64_t>(mod_inverse(static_cast<int64_t>(e_use), static_cast<int64_t>(phi)));

        std::string plain;
        for (uint64_t c : cipher) {
            plain.push_back(static_cast<char>(mod_pow(c, d, n) & 0xFF));
        }
        decrypted = std::move(plain);
        last_n = n; last_p = found_p; last_q = found_q; last_d = d;

        rep << "Key #" << (r + 1) << ": N=" << n << " factored -> p=" << found_p
            << ", q=" << found_q << ", d=" << d << " -> \"" << decrypted << "\"\r\n";

        int pct = 20 + static_cast<int>(((r + 1) * 80) / rounds);
        cb(pct, "[RSA-CRACKED #" + std::to_string(r + 1) + "/" + std::to_string(rounds) +
                "] Взломан N=" + std::to_string(n) + " (p=" + std::to_string(found_p) +
                ", q=" + std::to_string(found_q) + ") -> \"" + decrypted + "\"");
    }

    return {decrypted, last_n, (last_n > 100000ULL ? E : 17ULL), last_p, last_q, last_d, rep.str()};
}

// ============================================================================
//  4. REAL LZ77 -> BASE64 -> HEX -> ITU MORSE CODE -> DECODE ROUNDTRIP
// ============================================================================
namespace {

struct LZ77Tok {
    uint8_t offset;
    uint8_t length;
    uint8_t next;
};

std::vector<uint8_t> lz77_compress_bytes(const std::string& in) {
    std::vector<uint8_t> out;
    size_t i = 0;
    while (i < in.size()) {
        size_t best_len = 0;
        size_t best_off = 0;
        size_t win_start = (i > 32) ? (i - 32) : 0;
        for (size_t j = win_start; j < i; ++j) {
            size_t len = 0;
            while (i + len + 1 < in.size() && len < 15 && in[j + len] == in[i + len]) {
                ++len;
            }
            if (len > best_len) {
                best_len = len;
                best_off = i - j;
            }
        }
        out.push_back(static_cast<uint8_t>(best_off));
        out.push_back(static_cast<uint8_t>(best_len));
        out.push_back(static_cast<uint8_t>(in[i + best_len]));
        i += best_len + 1;
    }
    return out;
}

std::string lz77_decompress_bytes(const std::vector<uint8_t>& packed) {
    std::string out;
    for (size_t i = 0; i + 2 < packed.size(); i += 3) {
        size_t off = packed[i];
        size_t len = packed[i + 1];
        char nxt = static_cast<char>(packed[i + 2]);
        size_t start = out.size() - off;
        for (size_t k = 0; k < len; ++k) {
            out.push_back(out[start + k]);
        }
        out.push_back(nxt);
    }
    return out;
}

std::string base64_encode(const std::vector<uint8_t>& data) {
    static const char* B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(B64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(B64[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::vector<uint8_t> base64_decode(const std::string& in) {
    static const char* B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::array<int, 256> T{};
    T.fill(-1);
    for (int i = 0; i < 64; ++i) T[static_cast<uint8_t>(B64[i])] = i;
    std::vector<uint8_t> out;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

const std::unordered_map<char, std::string>& hex_to_morse_map() {
    static const std::unordered_map<char, std::string> m = {
        {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"},
        {'5', "....."}, {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."},
        {'A', ".-"},    {'B', "-..."},  {'C', "-.-."},  {'D', "-.."},   {'E', "."},
        {'F', "..-."}
    };
    return m;
}

} // namespace

CompressionMorseResult run_lz77_base64_morse_roundtrip(
    const std::string& input,
    const std::function<void(int pct, const std::string& log_msg)>& cb
) {
    auto lz = lz77_compress_bytes(input);
    std::string b64 = base64_encode(lz);

    // Base64 -> Upper Hex -> ITU Morse Code (case-safe!)
    std::ostringstream hex_ss;
    for (unsigned char c : b64) {
        hex_ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    std::string hex_str = hex_ss.str();

    const auto& m_enc = hex_to_morse_map();
    std::ostringstream morse_ss;
    for (size_t i = 0; i < hex_str.size(); ++i) {
        if (i > 0) morse_ss << ' ';
        morse_ss << m_enc.at(hex_str[i]);
    }
    std::string morse = morse_ss.str();

    cb(50, "[LZ77->B64->MORSE] Закодировано в Морзянку (" + std::to_string(morse.size()) + " точек/тире)");

    // Decode Morse -> Hex -> Base64 -> LZ77 -> original string!
    std::unordered_map<std::string, char> m_dec;
    for (const auto& [k, v] : m_enc) m_dec[v] = k;

    std::istringstream mis(morse);
    std::string tok;
    std::string dec_hex;
    while (mis >> tok) dec_hex.push_back(m_dec.at(tok));

    std::string dec_b64;
    for (size_t i = 0; i + 1 < dec_hex.size(); i += 2) {
        int byte_v = std::stoi(dec_hex.substr(i, 2), nullptr, 16);
        dec_b64.push_back(static_cast<char>(byte_v));
    }

    auto dec_lz = base64_decode(dec_b64);
    std::string recovered = lz77_decompress_bytes(dec_lz);

    std::ostringstream rep;
    rep << "\r\n=== LZ77 + BASE64 + HEX + ITU MORSE PIPELINE ===\r\n"
        << "LZ77 packed bytes : " << lz.size() << " (expanded from " << input.size() << "B for enterprise safety)\r\n"
        << "Base64            : " << b64 << "\r\n"
        << "Morse Code        : " << morse << "\r\n"
        << "Recovered Plain   : \"" << recovered << "\"\r\n";

    cb(100, "[MORSE->LZ77] Успешно декодировано обратно из Морзянки -> \"" + recovered + "\"");

    return {recovered, b64, morse, rep.str()};
}

} // namespace govno::madness
