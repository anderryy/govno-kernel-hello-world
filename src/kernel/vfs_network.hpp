#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace govno::kernel {

struct BlockchainMiningResult {
    uint64_t nonce = 0;
    uint64_t hashes_computed = 0;
    std::string block_hash_hex;
    std::string ledger_dump;
};

struct TcpDeliveryResult {
    std::vector<uint8_t> delivered_payload;
    std::string packet_trace_dump;
    size_t packet_count = 0;
};

BlockchainMiningResult mine_hello_world_blockchain_block(
    const std::vector<uint8_t>& gexe_payload,
    const std::function<void(const std::string& log_msg)>& log_cb
);

TcpDeliveryResult transmit_gexe_over_virtual_tcpip(
    const std::vector<uint8_t>& gexe_payload,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& progress_cb,
    const std::function<void(int ms)>& sleep_cb
);

} // namespace govno::kernel
