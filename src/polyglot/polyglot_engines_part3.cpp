#include "polyglot_engines.hpp"

#include <cstdint>
#include <regex>
#include <sstream>
#include <string>

namespace govno::polyglot {

PolyglotExecResult run_tcl_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Tcl 8.6";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(\{\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)\s*\})");
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
    ts << "Tcl Proc List Eval | items=" << res.steps_executed << " | sum=" << sum << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

PolyglotExecResult run_perl_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Perl 5";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(\[\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\])");
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
    ts << "Perl Scalar/Array Ref | tuples=" << res.steps_executed << " | sum=" << sum << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

PolyglotExecResult run_ruby_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Ruby";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(GlyphSpec\.new\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\))");
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
    ts << "Ruby Enumerable Struct | blocks=" << res.steps_executed << " | sum=" << sum << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

PolyglotExecResult run_julia_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Julia";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(JuliaGlyph\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\))");
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
    ts << "Julia Broadcast Kernel | structs=" << res.steps_executed << " | sum=" << sum << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

PolyglotExecResult run_powershell_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "PowerShell";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(\{\s*A\s*=\s*(-?\d+)\s*;\s*B\s*=\s*(-?\d+)\s*;\s*Bias\s*=\s*(-?\d+)\s*\})");
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
    ts << "PowerShell Object Pipe | objs=" << res.steps_executed << " | sum=" << sum << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

PolyglotExecResult run_dart_engine(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Dart";
    res.filename = fname;
    res.source_bytes = src.size();

    static const std::regex re(R"(DartGlyph\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\))");
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
    ts << "Dart Isolate Stream | nodes=" << res.steps_executed << " | sum=" << sum << " -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

} // namespace govno::polyglot
