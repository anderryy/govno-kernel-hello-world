#include "kernel/vfs_network.hpp"
#include "compiler/govno_compiler.hpp"
#include "madness/extreme_cs.hpp"

#include <algorithm>
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
    const std::function<void(const std::string& log_msg)>& log_cb
) {
    BlockchainMiningResult res;
    uint64_t nonce = 0;
    std::array<uint8_t, 32> digest{};
    std::vector<uint8_t> block_buf(gexe_payload.begin(), gexe_payload.end());
    block_buf.resize(gexe_payload.size() + 8);

    // Real FIPS-180-4 SHA-256 Proof-of-Work: require 12 leading zero bits (digest[0]==0 && (digest[1]&0xF0)==0)
    while (true) {
        for (int b = 0; b < 8; ++b) {
            block_buf[gexe_payload.size() + static_cast<size_t>(b)] =
                static_cast<uint8_t>((nonce >> (b * 8)) & 0xFFULL);
        }
        digest = govno::madness::sha256_bytes(block_buf.data(), block_buf.size());
        ++res.hashes_computed;
        if (digest[0] == 0x00 && (digest[1] & 0xF0) == 0x00) {
            break;
        }
        ++nonce;
    }

    res.nonce = nonce;
    std::ostringstream hs;
    hs << "0x" << std::hex << std::setfill('0');
    for (uint8_t byte : digest) {
        hs << std::setw(2) << static_cast<unsigned>(byte);
    }
    res.block_hash_hex = hs.str();

    std::ostringstream ld;
    ld << "; === GOVNO-CHAIN FIPS-180-4 SHA-256 PROOF-OF-WORK NOTARIZATION ===\r\n"
       << "Block #1337 | Difficulty: 0x000FFFFFFFFFFFFF (12 leading zero bits)\r\n"
       << "PrevHash: 0x000000000000deadbeef00000000000000000000000000000000000000000000\r\n"
       << "Nonce   : " << res.nonce << " (computed " << res.hashes_computed << " SHA-256 blocks)\r\n"
       << "SHA-256 : " << res.block_hash_hex << "\r\n"
       << "Payload : /bin/hello_world.gexe (" << gexe_payload.size() << " bytes)\r\n"
       << "Status  : CONFIRMED ON GOVNO-MAINNET (GAS FEE: 420 GOVNOCOIN)\r\n";
    res.ledger_dump = ld.str();

    log_cb("[SHA256-POW] Намайнен блок #1337 (nonce=" + std::to_string(res.nonce) +
           ", sha256=" + res.block_hash_hex.substr(0, 22) + "..., хешей=" + std::to_string(res.hashes_computed) + ")");
    return res;
}

TcpDeliveryResult transmit_gexe_over_virtual_tcpip(
    const std::vector<uint8_t>& gexe_payload,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& progress_cb,
    const std::function<void(int ms)>& sleep_cb
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
            sleep_cb(55);
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
