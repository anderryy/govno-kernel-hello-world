// ============================================================================
//  holyc2cpp_bootstrap.cpp — Build-Time HolyC-to-C++ Ahead-of-Time Transpiler
//  Compiled FIRST by build.bat, then compiles holyc/hello_temple.HC into C++
//  AND generates the cursed 32x32 ICO resource!
// ============================================================================

#include <cstdint>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

std::string unescape_str(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char n = s[i + 1];
            if (n == 'n') { out.push_back('\n'); ++i; continue; }
            if (n == 't') { out.push_back('\t'); ++i; continue; }
            if (n == '"') { out.push_back('"'); ++i; continue; }
            if (n == '\\') { out.push_back('\\'); ++i; continue; }
        }
        out.push_back(s[i]);
    }
    return out;
}

std::string expand_exe_blocks(const std::string& raw) {
    std::string res = raw;
    size_t pos = 0;
    while ((pos = res.find("#exe", pos)) != std::string::npos) {
        size_t open = res.find('{', pos);
        if (open == std::string::npos) break;
        int depth = 1;
        size_t p = open + 1;
        bool in_str = false;
        while (p < res.size() && depth > 0) {
            if (res[p] == '"' && (p == 0 || res[p - 1] != '\\')) in_str = !in_str;
            else if (!in_str) {
                if (res[p] == '{') ++depth;
                else if (res[p] == '}') --depth;
            }
            ++p;
        }
        if (depth != 0) break;
        std::string body = res.substr(open + 1, (p - 1) - (open + 1));
        std::ostringstream gen;
        size_t sp = 0;
        while ((sp = body.find("StreamPrint", sp)) != std::string::npos) {
            size_t q1 = body.find('"', sp);
            if (q1 == std::string::npos) break;
            size_t q2 = q1 + 1;
            while (q2 < body.size()) {
                if (body[q2] == '"' && body[q2 - 1] != '\\') break;
                ++q2;
            }
            if (q2 >= body.size()) break;
            gen << unescape_str(body.substr(q1 + 1, q2 - q1 - 1));
            sp = q2 + 1;
        }
        std::string code = gen.str();
        res.replace(pos, p - pos, code);
        pos += code.size();
    }
    return res;
}

std::string strip_doldoc(const std::string& s) {
    std::string out;
    bool in_str = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '"' && (i == 0 || s[i - 1] != '\\')) in_str = !in_str;
        if (!in_str && s[i] == '$') {
            size_t end = s.find('$', i + 1);
            if (end != std::string::npos) { i = end; continue; }
        }
        out.push_back(s[i]);
    }
    return out;
}

std::string transpile_holyc_to_cpp(const std::string& hc_expanded) {
    std::string s = strip_doldoc(hc_expanded);

    // Collect known 0-arg functions so "FnName;" becomes "FnName();"
    std::unordered_set<std::string> zero_arg_funcs;
    {
        std::regex fn_re(R"(\b(?:U0|I64|U8|F64)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\))");
        auto it = std::sregex_iterator(s.begin(), s.end(), fn_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) zero_arg_funcs.insert((*it)[1].str());
    }

    // Replace inline asm { MOV RAX, var \n INT 0x80 }
    {
        std::regex asm_re(R"(asm\s*\{\s*MOV\s+RAX\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*;?\s*INT\s+0x80\s*;?\s*\})");
        s = std::regex_replace(s, asm_re, "aot_emit_byte($1);");
    }

    // Replace HolyC backtick power "2`3" -> "holyc_ipow(2, 3)"
    {
        std::regex pow_re(R"((\b[A-Za-z0-9_]+)\s*`\s*([A-Za-z0-9_]+\b))");
        s = std::regex_replace(s, pow_re, "holyc_ipow($1, $2)");
    }

    // Replace HolyC chained comparisons "32 <= ch <= 126" -> "((32 <= ch) && (ch <= 126))"
    {
        std::regex chain_re(R"((\b[A-Za-z0-9_]+\b)\s*(<=|<|>=|>)\s*(\b[A-Za-z0-9_]+\b)\s*(<=|<|>=|>)\s*(\b[A-Za-z0-9_]+\b))");
        s = std::regex_replace(s, chain_re, "(($1 $2 $3) && ($3 $4 $5))");
    }

    // Replace HolyC types & Class keyword
    s = std::regex_replace(s, std::regex(R"(\bClass\b)"), "struct");
    s = std::regex_replace(s, std::regex(R"(\bU0\b)"), "void");
    s = std::regex_replace(s, std::regex(R"(\bI64\b)"), "int64_t");
    s = std::regex_replace(s, std::regex(R"(\bU8\b)"), "uint8_t");
    s = std::regex_replace(s, std::regex(R"(\bF64\b)"), "double");

    // Process line by line for bare string print statements and 0-arg parenthesis-free calls
    std::istringstream iss(s);
    std::ostringstream body_out;
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string trimmed = line;
        size_t first = trimmed.find_first_not_of(" \t");
        if (first != std::string::npos) trimmed = trimmed.substr(first);

        // Bare string statement: "...";
        if (!trimmed.empty() && trimmed.front() == '"' && trimmed.back() == ';') {
            std::string str_lit = trimmed.substr(0, trimmed.size() - 1);
            body_out << "  aot_print(" << str_lit << ");\n";
            continue;
        }
        // Parenthesis-free 0-arg call: FnName;
        if (!trimmed.empty() && trimmed.back() == ';') {
            std::string cand = trimmed.substr(0, trimmed.size() - 1);
            if (zero_arg_funcs.count(cand)) {
                body_out << "  " << cand << "();\n";
                continue;
            }
        }
        body_out << line << "\n";
    }

    std::string transpiled_code = body_out.str();

    std::ostringstream cpp;
    cpp << "// AUTO-GENERATED BY holyc2cpp.exe FROM holyc/hello_temple.HC AT BUILD TIME!\n"
        << "#include \"holyc_aot_bridge.hpp\"\n"
        << "#include <cstdint>\n"
        << "#include <string>\n\n"
        << "namespace govno::holyc_aot {\n\n"
        << "namespace {\n\n"
        << "thread_local std::string g_aot_emitted;\n"
        << "thread_local std::string g_aot_log;\n\n"
        << "int64_t holyc_ipow(int64_t base, int64_t exp) {\n"
        << "    int64_t r = 1;\n"
        << "    for (int64_t i = 0; i < exp; ++i) r *= base;\n"
        << "    return r;\n"
        << "}\n\n"
        << "void aot_print(const char* s) {\n"
        << "    for (size_t i = 0; s[i] != '\\0'; ++i) {\n"
        << "        if (s[i] == '$') {\n"
        << "            size_t j = i + 1;\n"
        << "            while (s[j] != '\\0' && s[j] != '$') ++j;\n"
        << "            if (s[j] == '$') { i = j; continue; }\n"
        << "        }\n"
        << "        g_aot_log.push_back(s[i]);\n"
        << "    }\n"
        << "}\n\n"
        << "void aot_emit_byte(int64_t b) {\n"
        << "    g_aot_emitted.push_back(static_cast<char>(b & 0xFF));\n"
        << "}\n\n"
        << "// Forward declarations for HolyC AOT functions\n"
        << "void BlessRingZero();\n"
        << "void EmitAotHello();\n"
        << "int64_t ValidatePrintableAscii(int64_t ch);\n"
        << "void TempleEmitChar(uint8_t ch);\n\n"
        << transpiled_code << "\n"
        << "} // namespace\n\n"
        << "HolyCAotResult run_compiled_holyc_aot() {\n"
        << "    g_aot_emitted.clear();\n"
        << "    g_aot_log.clear();\n"
        << "    Main();\n"
        << "    HolyCAotResult res;\n"
        << "    res.emitted_text = g_aot_emitted;\n"
        << "    res.aot_log = g_aot_log;\n"
        << "    res.magic_power = HOLY_SHIFT;\n"
        << "    res.check_val = HOLY_CHECK;\n"
        << "    return res;\n"
        << "}\n\n"
        << "const char* get_holyc_transpiled_cpp_preview() {\n"
        << "    return R\"HC_CPP(\n" << transpiled_code << "\n)HC_CPP\";\n"
        << "}\n\n"
        << "} // namespace govno::holyc_aot\n";

    return cpp.str();
}

void write_cursed_ico(const std::string& path) {
    // 32x32 32-bit BGRA ICO (radioactive lime + poop-gold smiley + red eyes)
    std::vector<uint8_t> ico;
    auto push_u16 = [&](uint16_t v) { ico.push_back(v & 0xFF); ico.push_back((v >> 8) & 0xFF); };
    auto push_u32 = [&](uint32_t v) {
        ico.push_back(v & 0xFF); ico.push_back((v >> 8) & 0xFF);
        ico.push_back((v >> 16) & 0xFF); ico.push_back((v >> 24) & 0xFF);
    };

    const uint32_t xor_bytes = 32 * 32 * 4;
    const uint32_t and_bytes = 32 * 4;
    const uint32_t img_size = 40 + xor_bytes + and_bytes;

    // ICONDIR
    push_u16(0); push_u16(1); push_u16(1);
    // ICONDIRENTRY
    ico.push_back(32); ico.push_back(32); ico.push_back(0); ico.push_back(0);
    push_u16(1); push_u16(32);
    push_u32(img_size);
    push_u32(22);

    // BITMAPINFOHEADER
    push_u32(40);
    push_u32(32);
    push_u32(64); // 2 * height
    push_u16(1);
    push_u16(32);
    push_u32(0);
    push_u32(xor_bytes);
    push_u32(0); push_u32(0); push_u32(0); push_u32(0);

    // Pixels (bottom-up BGRA)
    for (int y = 31; y >= 0; --y) {
        for (int x = 0; x < 32; ++x) {
            int dx = x - 16;
            int dy = y - 16;
            int r2 = dx * dx + dy * dy;
            uint8_t B = 0, G = 255, R = 0, A = 255; // neon green bg
            if ((x < 2 || x >= 30 || y < 2 || y >= 30)) {
                R = 255; G = 0; B = 255; // magenta border
            } else if (r2 <= 169) {
                R = 255; G = 220; B = 0; // yellow face
                if ((x >= 9 && x <= 12 && y >= 10 && y <= 14) ||
                    (x >= 20 && x <= 23 && y >= 10 && y <= 14)) {
                    R = 255; G = 0; B = 0; // cursed red eyes
                }
                if (y >= 20 && y <= 23 && x >= 10 && x <= 22) {
                    R = 20; G = 20; B = 20; // derpy grin
                }
            }
            ico.push_back(B); ico.push_back(G); ico.push_back(R); ico.push_back(A);
        }
    }
    // AND mask
    for (uint32_t i = 0; i < and_bytes; ++i) ico.push_back(0);

    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(ico.data()), static_cast<std::streamsize>(ico.size()));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: holyc2cpp <input.HC> <output.cpp> <output.ico>\n";
        return 1;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open input HolyC file: " << argv[1] << "\n";
        return 2;
    }
    std::ostringstream ss;
    ss << in.rdbuf();

    std::string expanded = expand_exe_blocks(ss.str());
    std::string cpp_code = transpile_holyc_to_cpp(expanded);

    std::ofstream out(argv[2], std::ios::binary);
    if (!out) {
        std::cerr << "Failed to write generated C++ file: " << argv[2] << "\n";
        return 3;
    }
    out << cpp_code;
    write_cursed_ico(argv[3]);

    std::cout << "[HOLYC2CPP-AOT] Compiled " << argv[1] << " -> " << argv[2]
              << " + generated cursed icon " << argv[3] << "\n";
    return 0;
}
