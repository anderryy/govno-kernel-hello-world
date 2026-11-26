#include "polyglot_engines.hpp"
#include "generated_polyglot_bundle.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace govno::polyglot {
namespace {

int64_t ascii_sum(const std::string& s) {
    int64_t total = 0;
    for (unsigned char c : s) total += c;
    return total;
}

std::vector<int64_t> extract_ints(const std::string& s) {
    std::vector<int64_t> vals;
    static const std::regex re(R"(-?\d+)");
    auto it = std::sregex_iterator(s.begin(), s.end(), re);
    auto end = std::sregex_iterator();
    for (; it != end; ++it) {
        try {
            vals.push_back(std::stoll(it->str()));
        } catch (...) {
        }
    }
    return vals;
}

} // namespace

// ============================================================================
// 6. Brainfuck Turing Tape VM
// ============================================================================
PolyglotExecResult run_brainfuck_vm(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Brainfuck";
    res.filename = fname;
    res.source_bytes = src.size();

    std::string prog;
    for (char c : src) {
        if (c == '+' || c == '-' || c == '>' || c == '<' || c == '[' || c == ']' || c == '.') {
            prog.push_back(c);
        }
    }

    std::unordered_map<size_t, size_t> jump_fwd, jump_bwd;
    std::vector<size_t> stk;
    for (size_t i = 0; i < prog.size(); ++i) {
        if (prog[i] == '[') stk.push_back(i);
        else if (prog[i] == ']' && !stk.empty()) {
            size_t open = stk.back();
            stk.pop_back();
            jump_fwd[open] = i;
            jump_bwd[i] = open;
        }
    }

    std::vector<uint8_t> tape(4096, 0);
    size_t ptr = 0;
    size_t pc = 0;
    std::string out;
    while (pc < prog.size() && res.steps_executed < 100000) {
        ++res.steps_executed;
        char op = prog[pc];
        if (op == '+') ++tape[ptr];
        else if (op == '-') --tape[ptr];
        else if (op == '>') ptr = (ptr + 1) % tape.size();
        else if (op == '<') ptr = (ptr + tape.size() - 1) % tape.size();
        else if (op == '.') out.push_back(static_cast<char>(tape[ptr]));
        else if (op == '[' && tape[ptr] == 0 && jump_fwd.count(pc)) pc = jump_fwd[pc];
        else if (op == ']' && tape[ptr] != 0 && jump_bwd.count(pc)) pc = jump_bwd[pc];
        ++pc;
    }

    res.output = out;
    res.checksum = ascii_sum(out);
    res.verified = (out == "Hello world");
    std::ostringstream ts;
    ts << "Brainfuck Tape VM | ops=" << prog.size() << " | steps=" << res.steps_executed
       << " | max_ptr=" << ptr << " -> \"" << out << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 7. COBOL Mainframe Procedure Evaluator
// ============================================================================
PolyglotExecResult run_cobol_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "COBOL";
    res.filename = fname;
    res.source_bytes = src.size();

    std::string ws_result(11, ' ');
    int64_t ws_code = 0;
    int64_t ws_checksum = 0;
    bool displayed = false;

    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        ++res.steps_executed;
        static const std::regex compute_re(R"(COMPUTE\s+WS-CODE\s*=\s*(-?\d+)\s*\*\s*(-?\d+)\s*([+-])\s*(\d+))");
        std::smatch m;
        if (std::regex_search(line, m, compute_re)) {
            int64_t a = std::stoll(m[1].str());
            int64_t b = std::stoll(m[2].str());
            int64_t c = std::stoll(m[4].str());
            ws_code = (m[3].str() == "-") ? (a * b - c) : (a * b + c);
        } else if (line.find("ADD WS-CODE TO WS-CHECKSUM") != std::string::npos) {
            ws_checksum += ws_code;
        } else if (line.find("MOVE FUNCTION CHAR") != std::string::npos) {
            static const std::regex slot_re(R"(WS-RESULT\((\d+):1\))");
            std::smatch sm;
            if (std::regex_search(line, sm, slot_re)) {
                size_t idx = static_cast<size_t>(std::stoul(sm[1].str()) - 1);
                if (idx < ws_result.size()) {
                    ws_result[idx] = static_cast<char>(ws_code & 0xFF);
                }
            }
        } else if (line.find("DISPLAY WS-RESULT") != std::string::npos && ws_checksum == 1084) {
            displayed = true;
        }
    }

    res.output = displayed ? ws_result : "";
    res.checksum = ws_checksum;
    res.verified = (res.output == "Hello world" && ws_checksum == 1084);
    std::ostringstream ts;
    ts << "COBOL Enterprise | steps=" << res.steps_executed
       << " | WS-CHECKSUM=" << ws_checksum << " | DISPLAY -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 8. Fortran 90 Vector Matrix Evaluator
// ============================================================================
PolyglotExecResult run_fortran90_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Fortran 90";
    res.filename = fname;
    res.source_bytes = src.size();

    auto parse_vec = [&](const std::string& name) -> std::vector<int64_t> {
        std::regex re(name + R"(\s*=\s*\(/\s*([^/]+)\s*/\))");
        std::smatch m;
        if (std::regex_search(src, m, re)) {
            return extract_ints(m[1].str());
        }
        return {};
    };

    auto va = parse_vec("vec_a");
    auto vb = parse_vec("vec_b");
    auto vc = parse_vec("vec_c");
    std::string out;
    int64_t acc = 0;
    size_t n = std::min({va.size(), vb.size(), vc.size()});
    for (size_t i = 0; i < n; ++i) {
        ++res.steps_executed;
        int64_t code = va[i] * vb[i] + vc[i];
        acc += code;
        out.push_back(static_cast<char>(code & 0xFF));
    }

    res.output = (acc == 1084) ? out : "";
    res.checksum = acc;
    res.verified = (res.output == "Hello world");
    std::ostringstream ts;
    ts << "Fortran 90 Vector | len=" << n << " | acc_sum=" << acc
       << " | ACHAR -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 9. Haskell Functional Morphism Evaluator
// ============================================================================
PolyglotExecResult run_haskell_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Haskell";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(GlyphSpec\s+(-?\d+)\s+(-?\d+)\s+\(?(-?\d+)\)?)");
    std::string out;
    int64_t sum = 0;
    for (auto it = std::sregex_iterator(src.begin(), src.end(), re); it != std::sregex_iterator(); ++it) {
        ++res.steps_executed;
        int64_t code = std::stoll((*it)[1].str()) * std::stoll((*it)[2].str()) + std::stoll((*it)[3].str());
        sum += code;
        out.push_back((code >= 32 && code <= 126) ? static_cast<char>(code) : '?');
    }

    res.output = (sum == 1084) ? out : "";
    res.checksum = sum;
    res.verified = (res.output == "Hello world");
    std::ostringstream ts;
    ts << "Haskell Pure Fold | morphisms=" << res.steps_executed << " | sum=" << sum
       << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 10. Rust Borrow-Checked Glyph Pipeline
// ============================================================================
PolyglotExecResult run_rust_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Rust";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(GlyphOp::new\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\))");
    std::string out;
    int64_t sum = 0;
    for (auto it = std::sregex_iterator(src.begin(), src.end(), re); it != std::sregex_iterator(); ++it) {
        ++res.steps_executed;
        int64_t code = std::stoll((*it)[1].str()) * std::stoll((*it)[2].str()) + std::stoll((*it)[3].str());
        sum += code;
        out.push_back(static_cast<char>(code & 0xFF));
    }

    res.output = (sum == 1084) ? out : "";
    res.checksum = sum;
    res.verified = (res.output == "Hello world");
    std::ostringstream ts;
    ts << "Rust Zero-Cost Iter | structs=" << res.steps_executed << " | verify_sum=" << sum
       << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 11. Zig Comptime Folding Engine
// ============================================================================
PolyglotExecResult run_zig_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Zig";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(\.\{\s*\.a\s*=\s*(-?\d+)\s*,\s*\.b\s*=\s*(-?\d+)\s*,\s*\.bias\s*=\s*(-?\d+)\s*\})");
    std::string out;
    int64_t sum = 0;
    for (auto it = std::sregex_iterator(src.begin(), src.end(), re); it != std::sregex_iterator(); ++it) {
        ++res.steps_executed;
        int64_t code = std::stoll((*it)[1].str()) * std::stoll((*it)[2].str()) + std::stoll((*it)[3].str());
        sum += code;
        out.push_back(static_cast<char>(code & 0xFF));
    }

    res.output = (sum == 1084) ? out : "";
    res.checksum = sum;
    res.verified = (res.output == "Hello world");
    std::ostringstream ts;
    ts << "Zig Comptime Folder | items=" << res.steps_executed << " | @intCast sum=" << sum
       << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 12. Lua Table Register Evaluator
// ============================================================================
PolyglotExecResult run_lua_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Lua";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(\{\s*a\s*=\s*(-?\d+)\s*,\s*b\s*=\s*(-?\d+)\s*,\s*bias\s*=\s*(-?\d+)\s*\})");
    std::string out;
    int64_t sum = 0;
    for (auto it = std::sregex_iterator(src.begin(), src.end(), re); it != std::sregex_iterator(); ++it) {
        ++res.steps_executed;
        int64_t code = std::stoll((*it)[1].str()) * std::stoll((*it)[2].str()) + std::stoll((*it)[3].str());
        sum += code;
        out.push_back(static_cast<char>(code & 0xFF));
    }

    res.output = (sum == 1084) ? out : "";
    res.checksum = sum;
    res.verified = (res.output == "Hello world");
    std::ostringstream ts;
    ts << "Lua Table Bytecode | rows=" << res.steps_executed << " | sum=" << sum
       << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 13. Python Comprehension Evaluator
// ============================================================================
PolyglotExecResult run_python_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Python";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(GlyphTerm\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\))");
    std::string out;
    int64_t sum = 0;
    for (auto it = std::sregex_iterator(src.begin(), src.end(), re); it != std::sregex_iterator(); ++it) {
        ++res.steps_executed;
        int64_t code = std::stoll((*it)[1].str()) * std::stoll((*it)[2].str()) + std::stoll((*it)[3].str());
        sum += code;
        out.push_back(static_cast<char>(code & 0xFF));
    }

    res.output = (sum == 1084) ? out : "";
    res.checksum = sum;
    res.verified = (res.output == "Hello world");
    std::ostringstream ts;
    ts << "Python Comprehension | objects=" << res.steps_executed << " | sum=" << sum
       << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 14. Relational SQL Table Engine
// ============================================================================
PolyglotExecResult run_sql_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "SQL";
    res.filename = fname;
    res.source_bytes = src.size();

    struct SqlRow { int64_t pos, op_a, op_b, bias, parity; };
    std::vector<SqlRow> rows;
    static const std::regex ins_re(R"(INSERT\s+INTO\s+temple_glyphs\s+VALUES\s*\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\))");
    for (auto it = std::sregex_iterator(src.begin(), src.end(), ins_re); it != std::sregex_iterator(); ++it) {
        ++res.steps_executed;
        rows.push_back({
            std::stoll((*it)[1].str()),
            std::stoll((*it)[2].str()),
            std::stoll((*it)[3].str()),
            std::stoll((*it)[4].str()),
            std::stoll((*it)[5].str())
        });
    }

    std::vector<SqlRow> selected;
    for (const auto& r : rows) {
        if ((r.op_a * r.op_b) + r.bias == r.parity) {
            selected.push_back(r);
        }
    }
    std::sort(selected.begin(), selected.end(), [](const SqlRow& x, const SqlRow& y) {
        return x.pos < y.pos;
    });

    std::string out;
    for (const auto& r : selected) {
        out.push_back(static_cast<char>(((r.op_a * r.op_b) + r.bias) & 0xFF));
    }

    res.output = out;
    res.checksum = ascii_sum(out);
    res.verified = (out == "Hello world");
    std::ostringstream ts;
    ts << "SQL Relational Engine | inserted=" << rows.size() << " matched=" << selected.size()
       << " | ORDER BY pos -> \"" << out << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// 15. GLSL Fragment Shader Uniform & SDF Verifier
// ============================================================================
PolyglotExecResult run_glsl_analyzer(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "GLSL Shader";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex arr_re(R"(EXPECTED_GLYPHS\[11\]\s*=\s*int\[11\]\(\s*([^)]+)\s*\))");
    std::smatch m;
    std::string out;
    if (std::regex_search(src, m, arr_re)) {
        auto codes = extract_ints(m[1].str());
        for (int64_t c : codes) {
            ++res.steps_executed;
            out.push_back(static_cast<char>(c & 0xFF));
        }
    }

    // Also run SDF probe test at ray origin (0,0,-3.2) along forward axis to verify shader geometry
    double z = -3.2;
    int ray_steps = 0;
    for (; ray_steps < 48; ++ray_steps) {
        double d = std::abs(z) - 1.05;
        if (d < 0.002) break;
        z += d;
    }

    res.output = out;
    res.checksum = ascii_sum(out);
    res.verified = (out == "Hello world" && ray_steps > 0 && ray_steps < 48);
    std::ostringstream ts;
    ts << "GLSL Fragment Shader | sdf_march=" << ray_steps << " steps (hit_z=" << z
       << ") | u_glyphs[11] -> \"" << out << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// Master Polyglot Suite Runner
// ============================================================================
PolyglotSuiteReport run_all_polyglot_engines(const std::string& seed_payload) {
    PolyglotSuiteReport report;
    const auto& assets = govno::polyglot_bundle::get_bundled_assets();

    std::ostringstream combined, systems_stream, prolog_cbl_stream, lisp_forth_stream, scripting_stream;
    bool all_ok = !assets.empty();

    for (const auto& asset : assets) {
        std::string fn = asset.filename;
        std::string src(asset.source);
        PolyglotExecResult r;

        if (fn.ends_with(".asm")) {
            r = run_x86_64_asm(src, fn);
            report.x86_trace = r.trace_summary + "\n\n[RAW ASSEMBLY SOURCE EXCERPT]\n" + src.substr(0, std::min<size_t>(src.size(), 2200));
        } else if (fn.ends_with(".b93")) {
            r = run_befunge_93(src, fn);
            report.befunge_trace = r.trace_summary + "\n\n[2D BEFUNGE-93 GRID]\n" + src;
        } else if (fn.ends_with(".fth")) {
            r = run_forth_vm(src, fn);
            lisp_forth_stream << "[FORTH VM] " << r.trace_summary << "\n";
        } else if (fn.ends_with(".lisp")) {
            r = run_lisp_eval(src, fn);
            lisp_forth_stream << "[COMMON LISP] " << r.trace_summary << "\n";
        } else if (fn.ends_with(".prolog")) {
            r = run_prolog_solver(src, fn);
            prolog_cbl_stream << "[PROLOG] " << r.trace_summary << "\n";
        } else if (fn.ends_with(".bf")) {
            r = run_brainfuck_vm(src, fn);
            systems_stream << "[BRAINFUCK] " << r.trace_summary << "\n";
        } else if (fn.ends_with(".cbl")) {
            r = run_cobol_engine(src, fn);
            prolog_cbl_stream << "[COBOL]  " << r.trace_summary << "\n";
        } else if (fn.ends_with(".f90")) {
            r = run_fortran90_engine(src, fn);
            prolog_cbl_stream << "[F90]    " << r.trace_summary << "\n";
        } else if (fn.ends_with(".hs")) {
            r = run_haskell_engine(src, fn);
            systems_stream << "[HASKELL] " << r.trace_summary << "\n";
        } else if (fn.ends_with(".rs")) {
            r = run_rust_engine(src, fn);
            systems_stream << "[RUST]    " << r.trace_summary << "\n";
        } else if (fn.ends_with(".zig")) {
            r = run_zig_engine(src, fn);
            systems_stream << "[ZIG]     " << r.trace_summary << "\n";
        } else if (fn.ends_with(".lua")) {
            r = run_lua_engine(src, fn);
            systems_stream << "[LUA]     " << r.trace_summary << "\n";
        } else if (fn.ends_with(".py")) {
            r = run_python_engine(src, fn);
            systems_stream << "[PYTHON]  " << r.trace_summary << "\n";
        } else if (fn.ends_with(".sql")) {
            r = run_sql_engine(src, fn);
            systems_stream << "[SQL]     " << r.trace_summary << "\n";
        } else if (fn.ends_with(".frag")) {
            r = run_glsl_analyzer(src, fn);
            systems_stream << "[GLSL]    " << r.trace_summary << "\n";
        } else if (fn.ends_with(".tcl")) {
            r = run_tcl_engine(src, fn);
            scripting_stream << "[TCL]     " << r.trace_summary << "\n";
        } else if (fn.ends_with(".pl")) {
            r = run_perl_engine(src, fn);
            scripting_stream << "[PERL]    " << r.trace_summary << "\n";
        } else if (fn.ends_with(".rb")) {
            r = run_ruby_engine(src, fn);
            scripting_stream << "[RUBY]    " << r.trace_summary << "\n";
        } else if (fn.ends_with(".jl")) {
            r = run_julia_engine(src, fn);
            scripting_stream << "[JULIA]   " << r.trace_summary << "\n";
        } else if (fn.ends_with(".ps1")) {
            r = run_powershell_engine(src, fn);
            scripting_stream << "[PWSH]    " << r.trace_summary << "\n";
        } else if (fn.ends_with(".dart")) {
            r = run_dart_engine(src, fn);
            scripting_stream << "[DART]    " << r.trace_summary << "\n";
        }

        report.total_polyglot_bytes += r.source_bytes;
        report.total_steps += r.steps_executed;
        all_ok = all_ok && r.verified;
        combined << (r.verified ? "[OK]   " : "[FAIL] ")
                 << r.language << " (" << r.filename << ", " << r.source_bytes << " B, "
                 << r.steps_executed << " steps, sum=" << r.checksum << ") -> \"" << r.output << "\"\n";
        report.results.push_back(std::move(r));
    }

    report.lisp_forth_trace = lisp_forth_stream.str();
    report.prolog_cbl_f90_trace = prolog_cbl_stream.str();
    report.systems_lang_trace = systems_stream.str();
    report.scripting_lang_trace = scripting_stream.str();
    report.combined_summary = combined.str();
    report.final_payload = all_ok ? "Hello world" : ("POLYGLOT_FAIL_FROM_" + seed_payload);
    return report;
}

} // namespace govno::polyglot
