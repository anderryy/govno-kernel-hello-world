#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace govno::polyglot {

struct PolyglotExecResult {
    std::string language;
    std::string filename;
    std::string output;
    std::string trace_summary;
    int64_t checksum = 0;
    size_t source_bytes = 0;
    size_t steps_executed = 0;
    bool verified = false;
};

struct PolyglotSuiteReport {
    std::string final_payload;
    std::string combined_summary;
    std::string x86_trace;
    std::string befunge_trace;
    std::string lisp_forth_trace;
    std::string prolog_cbl_f90_trace;
    std::string systems_lang_trace;
    std::vector<PolyglotExecResult> results;
    size_t total_polyglot_bytes = 0;
    size_t total_steps = 0;
};

PolyglotSuiteReport run_all_polyglot_engines(const std::string& seed_payload);

PolyglotExecResult run_x86_64_asm(const std::string& src, const std::string& fname);
PolyglotExecResult run_befunge_93(const std::string& src, const std::string& fname);
PolyglotExecResult run_forth_vm(const std::string& src, const std::string& fname);
PolyglotExecResult run_lisp_eval(const std::string& src, const std::string& fname);
PolyglotExecResult run_prolog_solver(const std::string& src, const std::string& fname);

PolyglotExecResult run_brainfuck_vm(const std::string& src, const std::string& fname);
PolyglotExecResult run_cobol_engine(const std::string& src, const std::string& fname);
PolyglotExecResult run_fortran90_engine(const std::string& src, const std::string& fname);
PolyglotExecResult run_haskell_engine(const std::string& src, const std::string& fname);
PolyglotExecResult run_rust_engine(const std::string& src, const std::string& fname);
PolyglotExecResult run_zig_engine(const std::string& src, const std::string& fname);
PolyglotExecResult run_lua_engine(const std::string& src, const std::string& fname);
PolyglotExecResult run_python_engine(const std::string& src, const std::string& fname);
PolyglotExecResult run_sql_engine(const std::string& src, const std::string& fname);
PolyglotExecResult run_glsl_analyzer(const std::string& src, const std::string& fname);

} // namespace govno::polyglot
