$FG,LTGREEN$// ============================================================================
//  10_holy_network_tcp.HC — IPv4 Checksums, TCP Window & Pigeon Routing in HolyC
//  Computes RFC-1071 IPv4 16-bit header checksums & routing tables in HolyC.
// ============================================================================$FG$

Class CHolyIpPacket {
  I64 ver_ihl;
  I64 total_len;
  I64 ttl;
  I64 proto;
  I64 src_ip_hi;
  I64 src_ip_lo;
  I64 dst_ip_hi;
  I64 dst_ip_lo;
  I64 checksum;
};

Class CHolyTcpConn {
  I64 seq;
  I64 ack;
  I64 state;
  I64 window_sz;
  I64 retransmits;
};

#exe {
  StreamPrint("I64 TCP_SYN = 2;\n");
  StreamPrint("I64 TCP_ACK = 16;\n");
  StreamPrint("I64 TCP_PSH = 8;\n");
  StreamPrint("I64 TCP_FIN = 1;\n");
}

I64 HolyComputeRfc1071Checksum(I64 *words, I64 word_cnt) {
  I64 sum = 0;
  I64 i = 0;
  while (i < word_cnt) {
    sum = sum + (words[i] % 65536);
    i = i + 1;
  }
  while (sum >= 65536) {
    sum = (sum % 65536) + (sum / 65536);
  }
  return 65535 - sum;
}

I64 HolySimulateTcpHandshake() {
  CHolyTcpConn conn;
  conn.seq = 1000;
  conn.ack = 0;
  conn.state = 0;
  conn.window_sz = 65535;
  conn.retransmits = 0;

  // SYN
  conn.state = 1;
  conn.seq = conn.seq + 1;
  // SYN-ACK
  conn.state = 2;
  conn.ack = 5001;
  // ESTABLISHED -> PSH+ACK 11 bytes of Hello world
  conn.state = 3;
  I64 bytes_sent = 0;
  I64 i = 0;
  while (i < 11) {
    conn.seq = conn.seq + 1;
    bytes_sent = bytes_sent + 1;
    i = i + 1;
  }
  // FIN
  conn.state = 4;
  return conn.seq + conn.ack + bytes_sent;
}

I64 HolyNetStackVerify() {
  "$FG,YELLOW$[HolyC Net] Computing RFC-1071 IPv4 Checksum & TCP State Machine...$FG$\n";
  I64 hdr[10];
  hdr[0] = 17664;
  hdr[1] = 51;
  hdr[2] = 4919;
  hdr[3] = 16384;
  hdr[4] = 16390;
  hdr[5] = 0;
  hdr[6] = 32512;
  hdr[7] = 1;
  hdr[8] = 2560;
  hdr[9] = 17706;

  I64 csum = HolyComputeRfc1071Checksum(hdr, 10);
  hdr[5] = csum;
  I64 verify = HolyComputeRfc1071Checksum(hdr, 10);
  I64 tcp_ok = HolySimulateTcpHandshake();
  return csum + verify + tcp_ok;
}
