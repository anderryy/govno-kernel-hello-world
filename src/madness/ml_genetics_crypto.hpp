#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace govno::madness {

struct NeuralTrainResult {
    std::string predicted_string;
    size_t epochs_run = 0;
    float final_mse = 0.0f;
    std::vector<float> loss_history;
    std::string report;
};

struct GeneticEvolveResult {
    std::string evolved_string;
    size_t generations = 0;
    std::string report;
};

struct RsaCrackResult {
    std::string decrypted_string;
    uint64_t modulus_n = 0;
    uint64_t public_e = 0;
    uint64_t cracked_p = 0;
    uint64_t cracked_q = 0;
    uint64_t recovered_d = 0;
    std::string report;
};

struct CompressionMorseResult {
    std::string recovered_string;
    std::string base64_payload;
    std::string morse_payload;
    std::string report;
};

NeuralTrainResult train_hello_world_neural_net(
    const std::string& training_target,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, float loss, const std::string& cur_pred, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
);

GeneticEvolveResult evolve_hello_world_genetics(
    const std::string& target,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
);

RsaCrackResult rsa_encrypt_and_crack(
    const std::string& input,
    const std::function<void(int pct, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
);

CompressionMorseResult run_lz77_base64_morse_roundtrip(
    const std::string& input,
    const std::function<void(int pct, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
);

} // namespace govno::madness
