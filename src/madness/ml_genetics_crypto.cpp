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
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, float loss, const std::string& cur_pred, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
) {
    const size_t N = training_target.size();
    constexpr size_t IN_DIM = 11;   // 10 Fourier sin/cos + 1 bias
    constexpr size_t HID_DIM = 18;
    constexpr float LR = 0.035f;

    std::mt19937 rng(133742);
    std::uniform_real_distribution<float> w_dist(-0.45f, 0.45f);

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
    rep << "=== GOVNO-GPT BACKPROP NEURAL NETWORK (11 -> 18 tanh -> 1) ===\r\n";

    std::string cur_pred(N, '?');
    float mse = 1.0f;

    for (size_t epoch = 0; epoch < 1200; ++epoch) {
        if (cancel_flag.load()) break;
        float total_sq_err = 0.0f;

        for (size_t s = 0; s < N; ++s) {
            auto x = make_features(s);
            float target = static_cast<float>(static_cast<uint8_t>(training_target[s])) / 128.0f;

            // Forward pass
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

            // Backward pass (SGD)
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

        mse = total_sq_err / static_cast<float>(std::max<size_t>(1, N));
        res.epochs_run = epoch + 1;
        res.final_mse = mse;

        if (epoch % 25 == 0 || cur_pred == training_target) {
            res.loss_history.push_back(mse);
        }

        if (epoch % 120 == 0 || (cur_pred == training_target && mse < 1e-5f)) {
            int pct = static_cast<int>(std::min<size_t>(100, ((epoch + 1) * 100) / 700));
            if (cur_pred == training_target && mse < 1e-5f) pct = 100;
            std::ostringstream line;
            line << "[NEURAL-SGD] Epoch " << std::setw(4) << (epoch + 1)
                 << " | MSE=" << std::fixed << std::setprecision(6) << mse
                 << " | Выход: \"" << cur_pred << "\"";
            rep << line.str() << "\r\n";
            cb(pct, mse, cur_pred, line.str());
            sleep_cb(85);
        }

        if (cur_pred == training_target && mse < 1e-5f && epoch >= 220) {
            break;
        }
    }

    res.predicted_string = cur_pred;
    rep << "Converged after " << res.epochs_run << " epochs -> \"" << res.predicted_string << "\"\r\n";
    res.report = rep.str();
    return res;
}

// ============================================================================
//  2. REAL GENETIC ALGORITHM EVOLUTION (DARWIN SELECTION + CROSSOVER + MUTATION)
// ============================================================================
GeneticEvolveResult evolve_hello_world_genetics(
    const std::string& target,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
) {
    const size_t len = target.size();
    constexpr size_t POP = 96;
    std::mt19937 rng(998877);
    std::uniform_int_distribution<int> char_dist(32, 122);
    std::uniform_int_distribution<size_t> idx_dist(0, POP - 1);
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);

    std::vector<std::string> pop(POP, std::string(len, ' '));
    for (auto& ind : pop) {
        for (size_t i = 0; i < len; ++i) ind[i] = static_cast<char>(char_dist(rng));
    }

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
    rep << "\r\n=== DARWIN GENETIC EVOLUTION (POP=" << POP << ") ===\r\n";

    std::string best = pop[0];
    for (size_t gen = 1; gen <= 400; ++gen) {
        if (cancel_flag.load()) break;
        std::sort(pop.begin(), pop.end(), [&](const std::string& a, const std::string& b) {
            return fitness(a) > fitness(b);
        });
        best = pop[0];
        res.generations = gen;

        if (gen == 1 || gen % 12 == 0 || best == target) {
            int pct = (best == target) ? 100 : static_cast<int>(std::min<size_t>(98, gen * 2));
            std::string msg = "[DARWIN-GA] Gen " + std::to_string(gen) +
                              " | Лучший мутант: \"" + best + "\" (fit=" + std::to_string(fitness(best)) + ")";
            rep << msg << "\r\n";
            cb(pct, msg);
            sleep_cb(75);
        }
        if (best == target) break;

        std::vector<std::string> next_pop;
        next_pop.reserve(POP);
        for (size_t e = 0; e < 6; ++e) next_pop.push_back(pop[e]);

        while (next_pop.size() < POP) {
            auto pick_parent = [&]() -> const std::string& {
                size_t best_i = idx_dist(rng);
                for (int t = 0; t < 3; ++t) {
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

    res.evolved_string = best;
    res.report = rep.str();
    return res;
}

// ============================================================================
//  3. REAL RSA ENCRYPTION + BRUTE-FORCE PRIME FACTORIZATION CRACKER
// ============================================================================
namespace {

uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1ULL) res = (res * base) % mod;
        base = (base * base) % mod;
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

} // namespace

RsaCrackResult rsa_encrypt_and_crack(
    const std::string& input,
    const std::function<void(int pct, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
) {
    constexpr uint64_t P = 257;
    constexpr uint64_t Q = 263;
    constexpr uint64_t N = P * Q; // 67591
    constexpr uint64_t E = 17;

    std::vector<uint64_t> cipher;
    cipher.reserve(input.size());
    for (unsigned char ch : input) {
        cipher.push_back(mod_pow(ch, E, N));
    }

    std::ostringstream rep;
    rep << "=== TOY RSA ENCRYPTION + BRUTE-FORCE FACTORIZATION CRACKER ===\r\n"
        << "Public Key (N=" << N << ", e=" << E << ")\r\nCiphertext: [ ";
    for (auto c : cipher) rep << c << " ";
    rep << "]\r\n";

    cb(30, "[RSA-ENCRYPT] Зашифровано " + std::to_string(cipher.size()) + " символов ключом N=" + std::to_string(N) + ", e=17");
    sleep_cb(130);

    // Deliberately forget private key and factor N by trial division!
    uint64_t found_p = 0;
    uint64_t found_q = 0;
    for (uint64_t cand = 3; cand * cand <= N; cand += 2) {
        if (N % cand == 0) {
            found_p = cand;
            found_q = N / cand;
            break;
        }
    }

    uint64_t phi = (found_p - 1) * (found_q - 1);
    uint64_t d = static_cast<uint64_t>(mod_inverse(static_cast<int64_t>(E), static_cast<int64_t>(phi)));

    std::string decrypted;
    for (uint64_t c : cipher) {
        decrypted.push_back(static_cast<char>(mod_pow(c, d, N) & 0xFF));
    }

    rep << "Brute-force factored N -> p=" << found_p << ", q=" << found_q
        << "\r\nRecovered phi=" << phi << ", private d=" << d
        << "\r\nDecrypted: \"" << decrypted << "\"\r\n";

    cb(100, "[RSA-CRACKER] Взломан собственный RSA! p=" + std::to_string(found_p) +
            ", q=" + std::to_string(found_q) + ", d=" + std::to_string(d) + " -> \"" + decrypted + "\"");
    sleep_cb(120);

    return {decrypted, N, E, found_p, found_q, d, rep.str()};
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
    const std::function<void(int pct, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
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
    sleep_cb(130);

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
    sleep_cb(120);

    return {recovered, b64, morse, rep.str()};
}

} // namespace govno::madness
