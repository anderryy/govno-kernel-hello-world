#include "madness/subleq_oisc.hpp"

#include <iomanip>
#include <sstream>
#include <vector>

namespace govno::madness {

SubleqRunResult compile_and_execute_subleq_oisc(const std::vector<uint8_t>& bytes) {
    // Each character b = q * 8 + r uses 8 instructions (24 int32_t words) + 1 HALT (3 words)
    const size_t n = bytes.size();
    const int32_t code_words = static_cast<int32_t>(n * 24 + 3);

    // Data segment layout starting at code_words
    const int32_t ZERO   = code_words + 0;
    const int32_t ACC    = code_words + 1;
    const int32_t CNT    = code_words + 2;
    const int32_t ONE    = code_words + 3;
    const int32_t NEG_8  = code_words + 4;
    const int32_t dyn_base = code_words + 5;

    std::vector<int32_t> mem(static_cast<size_t>(dyn_base + n * 2), 0);
    mem[ZERO] = 0;
    mem[ACC] = 0;
    mem[CNT] = 0;
    mem[ONE] = 1;
    mem[NEG_8] = -8;

    std::ostringstream dis;
    dis << "=== SUBLEQ (ONE-INSTRUCTION SET COMPUTER) OISC DISASSEMBLY ===\r\n"
        << "; Only instruction in CPU: subleq A, B, C  => mem[B] -= mem[A]; if (mem[B] <= 0) goto C\r\n";

    int32_t pc = 0;
    auto emit_subleq = [&](int32_t a, int32_t b, int32_t c, const char* comment) {
        if (pc < 150) {
            dis << std::setw(4) << pc << ": subleq " << std::setw(3) << a << ", "
                << std::setw(3) << b << ", " << std::setw(3) << c << "  ; " << comment << "\r\n";
        }
        mem[static_cast<size_t>(pc)] = a;
        mem[static_cast<size_t>(pc + 1)] = b;
        mem[static_cast<size_t>(pc + 2)] = c;
        pc += 3;
    };

    for (size_t i = 0; i < n; ++i) {
        int32_t val = static_cast<int32_t>(bytes[i]);
        int32_t q = val / 8;
        int32_t r = val % 8;
        int32_t neg_q = dyn_base + static_cast<int32_t>(i * 2);
        int32_t neg_r = neg_q + 1;
        mem[static_cast<size_t>(neg_q)] = -q;
        mem[static_cast<size_t>(neg_r)] = -r;

        int32_t base_pc = pc;
        int32_t loop_top = base_pc + 12;
        int32_t after_loop = base_pc + 21;

        emit_subleq(ACC, ACC, base_pc + 3, "ACC = 0");
        emit_subleq(CNT, CNT, base_pc + 6, "CNT = 0");
        emit_subleq(neg_q, CNT, base_pc + 9, "CNT = q");
        emit_subleq(ZERO, CNT, after_loop, "if CNT <= 0 break");
        // loop_top (base_pc + 12):
        emit_subleq(NEG_8, ACC, base_pc + 15, "ACC += 8");
        emit_subleq(ONE, CNT, after_loop, "CNT -= 1; if <= 0 break");
        emit_subleq(ZERO, ZERO, loop_top, "jmp loop_top");
        // after_loop (base_pc + 21):
        emit_subleq(neg_r, ACC, base_pc + 24, "ACC += rem");
        // Wait: base_pc + 24 is NEXT char!
        // Let's check exact count: 8 instructions * 3 = 24 words!
        // Oh! Let's recount: 0,3,6,9 (4 instr), 12,15,18 (3 instr), 21 (1 instr) = 8 instr = 24 words!
        // So wait: we also need OUT instruction! Let's make stride 27 words (9 instructions)!
    }

    // Rebuild cleanly with exact 27-word (9 instruction) stride per character
    const int32_t code_words2 = static_cast<int32_t>(n * 27 + 3);
    const int32_t Z2 = code_words2 + 0;
    const int32_t A2 = code_words2 + 1;
    const int32_t C2 = code_words2 + 2;
    const int32_t O2 = code_words2 + 3;
    const int32_t N8 = code_words2 + 4;
    const int32_t D2 = code_words2 + 5;

    mem.assign(static_cast<size_t>(D2 + n * 2), 0);
    mem[Z2] = 0; mem[A2] = 0; mem[C2] = 0; mem[O2] = 1; mem[N8] = -8;
    dis.str("");
    dis.clear();
    dis << "=== SUBLEQ (ONE-INSTRUCTION SET COMPUTER) OISC DISASSEMBLY ===\r\n"
        << "; CPU only has 1 opcode: subleq A, B, C => mem[B] -= mem[A]; if (mem[B] <= 0) goto C\r\n\r\n";

    pc = 0;
    auto emit2 = [&](int32_t a, int32_t b, int32_t c, const std::string& comment) {
        if (pc < 162) {
            dis << std::setw(4) << pc << ": subleq " << std::setw(3) << a << ", "
                << std::setw(3) << b << ", " << std::setw(3) << c << "  ; " << comment << "\r\n";
        }
        mem[static_cast<size_t>(pc)] = a;
        mem[static_cast<size_t>(pc + 1)] = b;
        mem[static_cast<size_t>(pc + 2)] = c;
        pc += 3;
    };

    for (size_t i = 0; i < n; ++i) {
        int32_t val = static_cast<int32_t>(bytes[i]);
        int32_t q = val / 8;
        int32_t r = val % 8;
        int32_t neg_q = D2 + static_cast<int32_t>(i * 2);
        int32_t neg_r = neg_q + 1;
        mem[static_cast<size_t>(neg_q)] = -q;
        mem[static_cast<size_t>(neg_r)] = -r;

        int32_t bp = pc;
        int32_t loop_top = bp + 12;
        int32_t after_loop = bp + 21;

        emit2(A2, A2, bp + 3, "ACC = 0");
        emit2(C2, C2, bp + 6, "CNT = 0");
        emit2(neg_q, C2, bp + 9, "CNT = " + std::to_string(q));
        emit2(Z2, C2, after_loop, "if CNT <= 0 skip loop");
        emit2(N8, A2, bp + 15, "ACC += 8");
        emit2(O2, C2, after_loop, "CNT -= 1; if <= 0 break");
        emit2(Z2, Z2, loop_top, "goto loop_top");
        emit2(neg_r, A2, bp + 24, "ACC += " + std::to_string(r));
        emit2(A2, -1, bp + 27, std::string("OUT port(-1) <- '") + static_cast<char>(val) + "'");
    }
    emit2(Z2, Z2, -1, "HALT");

    // Execute SUBLEQ OISC Virtual Machine!
    SubleqRunResult res;
    res.instructions_compiled = static_cast<size_t>(pc / 3);
    int32_t ip = 0;
    const size_t max_cycles = 100000;
    while (ip >= 0 && static_cast<size_t>(ip + 2) < mem.size() && res.cycles_executed < max_cycles) {
        int32_t a = mem[static_cast<size_t>(ip)];
        int32_t b = mem[static_cast<size_t>(ip + 1)];
        int32_t c = mem[static_cast<size_t>(ip + 2)];
        ++res.cycles_executed;

        if (b == -1) {
            res.output.push_back(static_cast<char>(mem[static_cast<size_t>(a)] & 0xFF));
            ip = c;
        } else {
            mem[static_cast<size_t>(b)] -= mem[static_cast<size_t>(a)];
            if (mem[static_cast<size_t>(b)] <= 0) {
                ip = c;
            } else {
                ip += 3;
            }
        }
    }

    dis << "\r\n... (" << res.instructions_compiled << " SUBLEQ instructions total)\r\n"
        << "SUBLEQ VM executed " << res.cycles_executed << " single-instruction cycles -> \""
        << res.output << "\"\r\n";
    res.disassembly_dump = dis.str();
    return res;
}

} // namespace govno::madness
