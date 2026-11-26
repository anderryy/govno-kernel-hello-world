// ============================================================================
//  holyc2cpp_bootstrap.cpp — Multi-Module Build-Time HolyC-to-C++20 Transpiler
//  Compiled FIRST during build, transpiles all holyc/*.HC files into C++20
//  and emits the 32x32 ICO resource.
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

std::string strip_doldoc_outside_strings(const std::string& s) {
    std::string out;
    bool in_str = false;
    bool in_line_cmt = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (in_line_cmt) {
            if (s[i] == '\n') in_line_cmt = false;
            out.push_back(s[i]);
            continue;
        }
        if (!in_str && s[i] == '/' && i + 1 < s.size() && s[i + 1] == '/') {
            in_line_cmt = true;
            out.push_back(s[i]);
            continue;
        }
        if (s[i] == '"' && (i == 0 || s[i - 1] != '\\')) {
            in_str = !in_str;
            out.push_back(s[i]);
            continue;
        }
        if (!in_str && s[i] == '$') {
            size_t end = s.find('$', i + 1);
            if (end != std::string::npos) { i = end; continue; }
        }
        out.push_back(s[i]);
    }
    return out;
}

std::string transpile_holyc_to_cpp(const std::string& hc_expanded, size_t total_hc_bytes) {
    std::string s = strip_doldoc_outside_strings(hc_expanded);

    // Collect 0-arg functions so "FnName;" becomes "FnName();"
    std::unordered_set<std::string> zero_arg_funcs;
    {
        std::regex fn0_re(R"(\b(?:U0|I64|U8|F64)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(\s*\))");
        auto it = std::sregex_iterator(s.begin(), s.end(), fn0_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) zero_arg_funcs.insert((*it)[1].str());
    }

    // Replace inline asm { MOV RAX, var; INT 0x80; }
    {
        std::regex asm_re(R"(asm\s*\{\s*MOV\s+RAX\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*;?\s*INT\s+0x80\s*;?\s*\})");
        s = std::regex_replace(s, asm_re, "aot_emit_byte($1);");
    }

    // Replace HolyC backtick power "2`3" -> "holyc_ipow(2, 3)"
    {
        std::regex pow_re(R"((\b[A-Za-z0-9_]+)\s*`\s*([A-Za-z0-9_]+\b))");
        s = std::regex_replace(s, pow_re, "holyc_ipow($1, $2)");
    }

    // Replace HolyC chained comparisons including array/member expressions
    {
        std::regex chain_re(R"(([A-Za-z0-9_]+(?:\[[^\]]+\]|\.[A-Za-z0-9_]+)*)\s*(<=|<|>=|>)\s*([A-Za-z0-9_]+(?:\[[^\]]+\]|\.[A-Za-z0-9_]+)*)\s*(<=|<|>=|>)\s*([A-Za-z0-9_]+(?:\[[^\]]+\]|\.[A-Za-z0-9_]+)*))");
        s = std::regex_replace(s, chain_re, "(($1 $2 $3) && ($3 $4 $5))");
    }

    // Replace HolyC types & Class keyword
    s = std::regex_replace(s, std::regex(R"(\bClass\b)"), "struct");
    s = std::regex_replace(s, std::regex(R"(\bU0\b)"), "void");
    s = std::regex_replace(s, std::regex(R"(\bI64\b)"), "int64_t");
    s = std::regex_replace(s, std::regex(R"(\bU8\b)"), "uint8_t");
    s = std::regex_replace(s, std::regex(R"(\bF64\b)"), "double");

    // Auto-extract struct forward declarations & function forward declarations
    std::vector<std::string> struct_decls;
    std::unordered_set<std::string> seen_structs;
    {
        std::regex st_re(R"(\bstruct\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{)");
        auto it = std::sregex_iterator(s.begin(), s.end(), st_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string nm = (*it)[1].str();
            if (!seen_structs.count(nm)) {
                seen_structs.insert(nm);
                struct_decls.push_back("struct " + nm + ";");
            }
        }
    }

    std::vector<std::string> fwd_decls;
    std::unordered_set<std::string> seen_decls;
    {
        std::regex decl_re(R"(\b([A-Za-z_][A-Za-z0-9_]*)[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]*\([^;{}\r\n]*\)[ \t]*(?=\{))");
        auto it = std::sregex_iterator(s.begin(), s.end(), decl_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string ret_t = (*it)[1].str();
            bool valid_ret = (ret_t == "void" || ret_t == "int64_t" || ret_t == "uint8_t" ||
                              ret_t == "double" || seen_structs.count(ret_t));
            if (!valid_ret) continue;
            std::string sig = it->str();
            if (!seen_decls.count(sig)) {
                seen_decls.insert(sig);
                fwd_decls.push_back(sig + ";");
            }
        }
    }

    // Line-by-line transform for bare string statements and 0-arg parenthesis-free calls
    std::istringstream iss(s);
    std::ostringstream body_out;
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string trimmed = line;
        size_t first = trimmed.find_first_not_of(" \t");
        if (first != std::string::npos) trimmed = trimmed.substr(first);

        if (!trimmed.empty() && trimmed.front() == '"' && trimmed.back() == ';') {
            std::string str_lit = trimmed.substr(0, trimmed.size() - 1);
            body_out << "  aot_print(" << str_lit << ");\n";
            continue;
        }
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
    std::string preview_snippet = transpiled_code.substr(0, std::min<size_t>(transpiled_code.size(), 8000));

    std::ostringstream cpp;
    cpp << "// AUTO-GENERATED BY holyc2cpp.exe FROM " << total_hc_bytes << " BYTES OF HOLYC SOURCE!\n"
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
        << "void aot_print_int(int64_t v) {\n"
        << "    g_aot_log += std::to_string(v);\n"
        << "}\n\n"
        << "void aot_emit_byte(int64_t b) {\n"
        << "    g_aot_emitted.push_back(static_cast<char>(b & 0xFF));\n"
        << "}\n\n"
        << "// Auto-extracted forward declarations from HolyC modules\n";

    for (const auto& sd : struct_decls) {
        cpp << sd << "\n";
    }
    for (const auto& fd : fwd_decls) {
        cpp << fd << "\n";
    }
    cpp << "\n" << transpiled_code << "\n"
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
        << "    return R\"HC_CPP(\n" << preview_snippet << "\n)HC_CPP\";\n"
        << "}\n\n"
        << "} // namespace govno::holyc_aot\n";

    return cpp.str();
}

void write_cursed_ico(const std::string& path) {
    std::vector<uint8_t> ico;
    auto push_u16 = [&](uint16_t v) { ico.push_back(v & 0xFF); ico.push_back((v >> 8) & 0xFF); };
    auto push_u32 = [&](uint32_t v) {
        ico.push_back(v & 0xFF); ico.push_back((v >> 8) & 0xFF);
        ico.push_back((v >> 16) & 0xFF); ico.push_back((v >> 24) & 0xFF);
    };

    const uint32_t xor_bytes = 32 * 32 * 4;
    const uint32_t and_bytes = 32 * 4;
    const uint32_t img_size = 40 + xor_bytes + and_bytes;

    push_u16(0); push_u16(1); push_u16(1);
    ico.push_back(32); ico.push_back(32); ico.push_back(0); ico.push_back(0);
    push_u16(1); push_u16(32);
    push_u32(img_size);
    push_u32(22);

    push_u32(40); push_u32(32); push_u32(64);
    push_u16(1); push_u16(32); push_u32(0);
    push_u32(xor_bytes);
    push_u32(0); push_u32(0); push_u32(0); push_u32(0);

    for (int y = 31; y >= 0; --y) {
        for (int x = 0; x < 32; ++x) {
            int dx = x - 16, dy = y - 16;
            int r2 = dx * dx + dy * dy;
            uint8_t B = 0, G = 255, R = 0, A = 255;
            if (x < 2 || x >= 30 || y < 2 || y >= 30) {
                R = 255; G = 0; B = 255;
            } else if (r2 <= 169) {
                R = 255; G = 220; B = 0;
                if ((x >= 9 && x <= 12 && y >= 10 && y <= 14) ||
                    (x >= 20 && x <= 23 && y >= 10 && y <= 14)) {
                    R = 255; G = 0; B = 0;
                }
                if (y >= 20 && y <= 23 && x >= 10 && x <= 22) {
                    R = 20; G = 20; B = 20;
                }
            }
            ico.push_back(B); ico.push_back(G); ico.push_back(R); ico.push_back(A);
        }
    }
    for (uint32_t i = 0; i < and_bytes; ++i) ico.push_back(0);

    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(ico.data()), static_cast<std::streamsize>(ico.size()));
}

std::string detect_polyglot_language(const std::string& filename) {
    auto dot = filename.rfind('.');
    std::string ext = (dot != std::string::npos) ? filename.substr(dot) : "";
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (ext == ".asm") return "Assembly";
    if (ext == ".b93") return "Befunge-93";
    if (ext == ".fth") return "Forth";
    if (ext == ".lisp") return "Common Lisp";
    if (ext == ".prolog" || ext == ".pro") return "Prolog";
    if (ext == ".bf") return "Brainfuck";
    if (ext == ".cbl") return "COBOL";
    if (ext == ".f90") return "Fortran";
    if (ext == ".hs") return "Haskell";
    if (ext == ".rs") return "Rust";
    if (ext == ".zig") return "Zig";
    if (ext == ".lua") return "Lua";
    if (ext == ".py") return "Python";
    if (ext == ".sql") return "SQL";
    if (ext == ".frag") return "GLSL";
    if (ext == ".tcl") return "Tcl";
    if (ext == ".pl") return "Perl";
    if (ext == ".rb") return "Ruby";
    if (ext == ".jl") return "Julia";
    if (ext == ".ps1") return "PowerShell";
    if (ext == ".dart") return "Dart";
    return "Unknown";
}

int bundle_polyglot_files(const std::string& out_hpp, int argc, char** argv, int start_idx) {
    std::ostringstream hpp;
    hpp << "// AUTO-GENERATED POLYGLOT ASSET BUNDLE BY holyc2cpp_bootstrap\n"
        << "#pragma once\n"
        << "#include <cstddef>\n"
        << "#include <vector>\n\n"
        << "namespace govno::polyglot_bundle {\n\n"
        << "struct PolyglotAsset {\n"
        << "    const char* filename;\n"
        << "    const char* language;\n"
        << "    const char* source;\n"
        << "    size_t byte_size;\n"
        << "};\n\n"
        << "inline const std::vector<PolyglotAsset>& get_bundled_assets() {\n"
        << "    static const std::vector<PolyglotAsset> kAssets = {\n";

    size_t total_poly_bytes = 0;
    for (int i = start_idx; i < argc; ++i) {
        std::string path = argv[i];
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            std::cerr << "Failed to open polyglot file: " << path << "\n";
            return 4;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        std::string raw = ss.str();
        std::string normalized;
        normalized.reserve(raw.size());
        for (char c : raw) {
            if (c != '\r') normalized.push_back(c);
        }
        total_poly_bytes += normalized.size();
        auto slash = path.find_last_of("\\/");
        std::string base = (slash != std::string::npos) ? path.substr(slash + 1) : path;
        std::string lang = detect_polyglot_language(base);
        hpp << "        {\n"
            << "            \"" << base << "\",\n"
            << "            \"" << lang << "\",\n"
            << "            R\"GOVNO_POLY(" << normalized << ")GOVNO_POLY\",\n"
            << "            " << normalized.size() << "\n"
            << "        },\n";
    }

    hpp << "    };\n"
        << "    return kAssets;\n"
        << "}\n\n"
        << "} // namespace govno::polyglot_bundle\n";

    std::ofstream out(out_hpp, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to write polyglot bundle header: " << out_hpp << "\n";
        return 5;
    }
    out << hpp.str();
    std::cout << "[HOLYC2CPP-BUNDLE] Bundled " << (argc - start_idx)
              << " polyglot files (" << total_poly_bytes << " bytes) -> " << out_hpp << "\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc >= 4 && std::string(argv[1]) == "--bundle-polyglot") {
        return bundle_polyglot_files(argv[2], argc, argv, 3);
    }

    if (argc < 4) {
        std::cerr << "Usage: holyc2cpp <output.cpp> <output.ico> <in1.HC> [in2.HC ...]\n"
                  << "   or: holyc2cpp --bundle-polyglot <out.hpp> <poly1> [poly2 ...]\n";
        return 1;
    }

    std::ostringstream combined_hc;
    size_t total_bytes = 0;
    for (int i = 3; i < argc; ++i) {
        std::ifstream in(argv[i], std::ios::binary);
        if (!in) {
            std::cerr << "Failed to open input HolyC file: " << argv[i] << "\n";
            return 2;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        std::string txt = ss.str();
        total_bytes += txt.size();
        combined_hc << "// --- MODULE: " << argv[i] << " ---\n" << txt << "\n";
    }

    std::string expanded = expand_exe_blocks(combined_hc.str());
    std::string cpp_code = transpile_holyc_to_cpp(expanded, total_bytes);

    std::ofstream out(argv[1], std::ios::binary);
    if (!out) {
        std::cerr << "Failed to write generated C++ file: " << argv[1] << "\n";
        return 3;
    }
    out << cpp_code;
    write_cursed_ico(argv[2]);

    std::cout << "[HOLYC2CPP-AOT] Transpiled " << (argc - 3) << " HolyC modules ("
              << total_bytes << " bytes) -> " << argv[1] << "\n";
    return 0;
}
