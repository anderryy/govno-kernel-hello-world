#include "kernel/vfs_network.hpp"
#include "compiler/govno_compiler.hpp"
#include "madness/extreme_cs.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace govno::kernel {

namespace {

uint16_t ipv4_header_checksum(const uint8_t* hdr20) {
    uint32_t sum = 0;
    for (int i = 0; i < 20; i += 2) {
        uint16_t word = (static_cast<uint16_t>(hdr20[i]) << 8) | hdr20[i + 1];
        sum += word;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum & 0xFFFFu);
}

struct VirtualPacket {
    std::string direction;
    std::string flags;
    uint32_t seq = 0;
    uint32_t ack = 0;
    uint16_t ip_checksum = 0;
    std::vector<uint8_t> eth_ip_tcp_frame;
    std::vector<uint8_t> tcp_payload;
};

VirtualPacket build_virtual_tcp_packet(
    const std::string& direction,
    const std::string& flags,
    uint32_t src_ip,
    uint32_t dst_ip,
    uint16_t src_port,
    uint16_t dst_port,
    uint32_t seq,
    uint32_t ack,
    const std::vector<uint8_t>& payload
) {
    VirtualPacket pkt;
    pkt.direction = direction;
    pkt.flags = flags;
    pkt.seq = seq;
    pkt.ack = ack;
    pkt.tcp_payload = payload;

    std::vector<uint8_t> frame;
    // 14-byte Ethernet header (fake MACs)
    const uint8_t eth[14] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0x69, 0x42,
        0xCA, 0xFE, 0xBA, 0xBE, 0x13, 0x37,
        0x08, 0x00
    };
    frame.insert(frame.end(), eth, eth + 14);

    // 20-byte IPv4 header
    uint16_t total_len = static_cast<uint16_t>(20 + 20 + payload.size());
    uint8_t ip[20] = {
        0x45, 0x00,
        static_cast<uint8_t>(total_len >> 8), static_cast<uint8_t>(total_len & 0xFF),
        0x13, 0x37, 0x40, 0x00,
        64, 6, 0x00, 0x00,
        static_cast<uint8_t>((src_ip >> 24) & 0xFF), static_cast<uint8_t>((src_ip >> 16) & 0xFF),
        static_cast<uint8_t>((src_ip >> 8) & 0xFF), static_cast<uint8_t>(src_ip & 0xFF),
        static_cast<uint8_t>((dst_ip >> 24) & 0xFF), static_cast<uint8_t>((dst_ip >> 16) & 0xFF),
        static_cast<uint8_t>((dst_ip >> 8) & 0xFF), static_cast<uint8_t>(dst_ip & 0xFF)
    };
    uint16_t csum = ipv4_header_checksum(ip);
    ip[10] = static_cast<uint8_t>(csum >> 8);
    ip[11] = static_cast<uint8_t>(csum & 0xFF);
    pkt.ip_checksum = csum;
    frame.insert(frame.end(), ip, ip + 20);

    // 20-byte TCP header
    uint8_t tcp[20] = {
        static_cast<uint8_t>(src_port >> 8), static_cast<uint8_t>(src_port & 0xFF),
        static_cast<uint8_t>(dst_port >> 8), static_cast<uint8_t>(dst_port & 0xFF),
        static_cast<uint8_t>((seq >> 24) & 0xFF), static_cast<uint8_t>((seq >> 16) & 0xFF),
        static_cast<uint8_t>((seq >> 8) & 0xFF), static_cast<uint8_t>(seq & 0xFF),
        static_cast<uint8_t>((ack >> 24) & 0xFF), static_cast<uint8_t>((ack >> 16) & 0xFF),
        static_cast<uint8_t>((ack >> 8) & 0xFF), static_cast<uint8_t>(ack & 0xFF),
        0x50, 0x18, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00
    };
    frame.insert(frame.end(), tcp, tcp + 20);
    frame.insert(frame.end(), payload.begin(), payload.end());
    pkt.eth_ip_tcp_frame = std::move(frame);
    return pkt;
}

} // namespace

BlockchainMiningResult mine_hello_world_blockchain_block(
    const std::vector<uint8_t>& gexe_payload,
    bool heavy_mode,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, uint64_t total_hashes, const std::string& log_msg)>& progress_cb
) {
    BlockchainMiningResult res;
    const size_t num_blocks = heavy_mode ? 6 : 1;
    const int diff_bits = heavy_mode ? 20 : 12;

    std::string prev_hash = "0x000000000000deadbeef00000000000000000000000000000000000000000000";
    std::ostringstream ld;
    ld << "; === GOVNO-CHAIN FIPS-180-4 SHA-256 MERKLE & MULTI-BLOCK PROOF-OF-WORK ===\r\n"
       << "Blocks: " << num_blocks << " | Difficulty: " << diff_bits << " leading zero bits\r\n\r\n";

    // Compute Merkle Root of 512 synthetic transactions
    std::vector<std::array<uint8_t, 32>> merkle_layer(heavy_mode ? 1024 : 32);
    for (size_t i = 0; i < merkle_layer.size(); ++i) {
        std::vector<uint8_t> tx = gexe_payload;
        tx.push_back(static_cast<uint8_t>(i & 0xFF));
        tx.push_back(static_cast<uint8_t>((i >> 8) & 0xFF));
        merkle_layer[i] = govno::madness::sha256_bytes(tx.data(), tx.size());
    }
    while (merkle_layer.size() > 1) {
        std::vector<std::array<uint8_t, 32>> next_layer;
        next_layer.reserve(merkle_layer.size() / 2);
        for (size_t i = 0; i + 1 < merkle_layer.size(); i += 2) {
            std::array<uint8_t, 64> pair_buf{};
            std::memcpy(pair_buf.data(), merkle_layer[i].data(), 32);
            std::memcpy(pair_buf.data() + 32, merkle_layer[i + 1].data(), 32);
            next_layer.push_back(govno::madness::sha256_bytes(pair_buf.data(), pair_buf.size()));
        }
        merkle_layer = std::move(next_layer);
    }

    const auto t_mine_start = std::chrono::steady_clock::now();

    for (size_t blk = 0; blk < num_blocks; ++blk) {
        if (cancel_flag.load()) break;

        std::vector<uint8_t> block_buf(gexe_payload.begin(), gexe_payload.end());
        block_buf.insert(block_buf.end(), prev_hash.begin(), prev_hash.end());
        block_buf.insert(block_buf.end(), merkle_layer[0].begin(), merkle_layer[0].end());
        size_t nonce_off = block_buf.size();
        block_buf.resize(nonce_off + 8);

        uint64_t nonce = 0;
        std::array<uint8_t, 32> digest{};
        while (true) {
            if ((nonce & 0x7FFFULL) == 0) {
                if (cancel_flag.load()) break;
                if (nonce > 0) {
                    auto now = std::chrono::steady_clock::now();
                    double sec = std::max(0.001, std::chrono::duration<double>(now - t_mine_start).count());
                    uint64_t hps = static_cast<uint64_t>(static_cast<double>(res.hashes_computed) / sec);
                    int pct = static_cast<int>(((blk * 100) / num_blocks) + (50 / num_blocks));
                    progress_cb(std::min(99, pct), res.hashes_computed,
                                "[SHA256-MINER] Блок #" + std::to_string(1337 + blk) +
                                " | Перебрано " + std::to_string(res.hashes_computed) +
                                " SHA-256 (" + std::to_string(hps / 1000ULL) + " kH/s)...");
                }
            }
            for (int b = 0; b < 8; ++b) {
                block_buf[nonce_off + static_cast<size_t>(b)] =
                    static_cast<uint8_t>((nonce >> (b * 8)) & 0xFFULL);
            }
            digest = govno::madness::sha256_bytes(block_buf.data(), block_buf.size());
            ++res.hashes_computed;

            bool match = heavy_mode
                ? (digest[0] == 0x00 && digest[1] == 0x00 && (digest[2] & 0xF0) == 0x00)
                : (digest[0] == 0x00 && (digest[1] & 0xF0) == 0x00);
            if (match) break;
            ++nonce;
        }
        if (cancel_flag.load()) break;

        res.nonce = nonce;
        std::ostringstream hs;
        hs << "0x" << std::hex << std::setfill('0');
        for (uint8_t byte : digest) hs << std::setw(2) << static_cast<unsigned>(byte);
        res.block_hash_hex = hs.str();

        ld << "Block #" << (1337 + blk) << " | Nonce=" << nonce
           << "\r\n  PrevHash: " << prev_hash
           << "\r\n  SHA-256 : " << res.block_hash_hex << "\r\n\r\n";

        int pct = static_cast<int>(((blk + 1) * 100) / num_blocks);
        progress_cb(pct, res.hashes_computed,
                    "[SHA256-POW] Блок #" + std::to_string(1337 + blk) + " добыт! nonce=" +
                    std::to_string(nonce) + " -> " + res.block_hash_hex.substr(0, 20) + "...");
        prev_hash = res.block_hash_hex;
    }

    ld << "Total SHA-256 compressions: " << res.hashes_computed
       << "\r\nStatus: CONFIRMED ON GOVNO-MAINNET (GAS FEE: 420 GOVNOCOIN)\r\n";
    res.ledger_dump = ld.str();
    return res;
}

TcpDeliveryResult transmit_gexe_over_virtual_tcpip(
    const std::vector<uint8_t>& gexe_payload,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& progress_cb
) {
    TcpDeliveryResult out;
    std::vector<VirtualPacket> wire_ring;

    const uint32_t cli_ip = (127u << 24) | 1u;
    const uint32_t srv_ip = (10u << 24) | (69u << 8) | 42u;

    // Step 1: 3-Way TCP Handshake
    wire_ring.push_back(build_virtual_tcp_packet("127.0.0.1:51337 -> 10.0.69.42:8080", "SYN", cli_ip, srv_ip, 51337, 8080, 1000, 0, {}));
    wire_ring.push_back(build_virtual_tcp_packet("10.0.69.42:8080 -> 127.0.0.1:51337", "SYN+ACK", srv_ip, cli_ip, 8080, 51337, 5000, 1001, {}));
    wire_ring.push_back(build_virtual_tcp_packet("127.0.0.1:51337 -> 10.0.69.42:8080", "ACK", cli_ip, srv_ip, 51337, 8080, 1001, 5001, {}));

    // Step 2: Fragment .gexe inside HTTP/1.1 POST across 32-byte TCP MTU segments (for maximum bloat!)
    const std::string http_hdr =
        "POST /api/v2/kernel-exec HTTP/1.1\r\n"
        "Host: localhost.govno\r\n"
        "Content-Type: application/x-govno-gexe\r\n\r\n";

    std::vector<uint8_t> http_stream(http_hdr.begin(), http_hdr.end());
    http_stream.insert(http_stream.end(), gexe_payload.begin(), gexe_payload.end());

    uint32_t seq = 1001;
    const size_t mtu = 32;
    for (size_t off = 0; off < http_stream.size(); off += mtu) {
        size_t chunk_len = (off + mtu < http_stream.size()) ? mtu : (http_stream.size() - off);
        std::vector<uint8_t> seg(http_stream.begin() + off, http_stream.begin() + off + chunk_len);
        wire_ring.push_back(build_virtual_tcp_packet(
            "127.0.0.1:51337 -> 10.0.69.42:8080", "PSH+ACK",
            cli_ip, srv_ip, 51337, 8080, seq, 5001, seg
        ));
        seq += static_cast<uint32_t>(chunk_len);
    }

    const std::string http_ok = "HTTP/1.1 200 OK\r\nX-Govno-Status: SUFFERING_ACCEPTED\r\n\r\n";
    wire_ring.push_back(build_virtual_tcp_packet(
        "10.0.69.42:8080 -> 127.0.0.1:51337", "PSH+ACK",
        srv_ip, cli_ip, 8080, 51337, 5001, seq,
        std::vector<uint8_t>(http_ok.begin(), http_ok.end())
    ));
    wire_ring.push_back(build_virtual_tcp_packet("127.0.0.1:51337 -> 10.0.69.42:8080", "FIN+ACK", cli_ip, srv_ip, 51337, 8080, seq, 5001 + static_cast<uint32_t>(http_ok.size()), {}));

    // Reassemble client payload back from Ethernet/IP/TCP wire frames!
    std::vector<uint8_t> reassembled_http;
    std::ostringstream trace;
    trace << "=== GOVNO-NET VIRTUAL WIRESHARK CAPTURE (" << wire_ring.size() << " FRAMES) ===\r\n";

    for (size_t i = 0; i < wire_ring.size(); ++i) {
        if (cancel_flag.load()) break;
        const auto& p = wire_ring[i];
        trace << "#" << std::setw(2) << i << " " << p.direction
              << " [" << p.flags << "] seq=" << p.seq << " ack=" << p.ack
              << " ip_csum=0x" << std::hex << std::setw(4) << std::setfill('0') << p.ip_checksum
              << std::dec << " frame=" << p.eth_ip_tcp_frame.size() << "B payload=" << p.tcp_payload.size() << "B\r\n";

        if (p.direction.rfind("127.0.0.1", 0) == 0 && !p.tcp_payload.empty()) {
            reassembled_http.insert(reassembled_http.end(), p.tcp_payload.begin(), p.tcp_payload.end());
        }

        if (i == 0 || i == 2 || i == wire_ring.size() - 2 || (i % 3 == 0)) {
            int pct = static_cast<int>(((i + 1) * 100) / wire_ring.size());
            progress_cb(pct, "[GOVNO-NET] Пакет #" + std::to_string(i) + " [" + p.flags + "] " + p.direction +
                             " (csum=0x" + ([&]() {
                                 std::ostringstream s;
                                 s << std::hex << std::setw(4) << std::setfill('0') << p.ip_checksum;
                                 return s.str();
                             })() + ")");
        }
    }

    // Strip HTTP headers after "\r\n\r\n" to recover raw .gexe executable!
    const std::string sep = "\r\n\r\n";
    auto it = std::search(reassembled_http.begin(), reassembled_http.end(), sep.begin(), sep.end());
    if (it != reassembled_http.end()) {
        out.delivered_payload.assign(it + sep.size(), reassembled_http.end());
    } else {
        out.delivered_payload = gexe_payload;
    }

    out.packet_trace_dump = trace.str();
    out.packet_count = wire_ring.size();
    return out;
}

} // namespace govno::kernel
