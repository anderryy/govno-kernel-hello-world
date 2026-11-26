#include "polyglot_engines.hpp"
#include "generated_polyglot_bundle.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace govno::polyglot {
namespace {

std::string trim_copy(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string to_lower_copy(std::string s) {
    for (char& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}

int64_t calc_ascii_sum(const std::string& s) {
    int64_t acc = 0;
    for (unsigned char c : s) acc += c;
    return acc;
}

// ============================================================================
// 1. Real x86-64 Assembly Emulator
// ============================================================================
struct AsmInsn {
    std::string op;
    std::string arg1;
    std::string arg2;
};

int64_t parse_imm(const std::string& tok) {
    std::string t = trim_copy(tok);
    if (t.empty()) return 0;
    try {
        return std::stoll(t, nullptr, 0);
    } catch (...) {
        return 0;
    }
}

bool is_numeric_tok(const std::string& tok) {
    std::string t = trim_copy(tok);
    if (t.empty()) return false;
    size_t idx = (t[0] == '-' || t[0] == '+') ? 1 : 0;
    return idx < t.size() && std::isdigit(static_cast<unsigned char>(t[idx]));
}

// ============================================================================
// 4. Lisp S-Expression AST
// ============================================================================
struct SExpr {
    enum class Kind { NUMBER, SYMBOL, LIST } kind = Kind::SYMBOL;
    int64_t num = 0;
    std::string sym;
    std::vector<SExpr> list;
};

std::vector<std::string> lisp_tokenize(const std::string& src) {
    std::vector<std::string> toks;
    std::istringstream iss(src);
    std::string line;
    std::string clean;
    while (std::getline(iss, line)) {
        auto semi = line.find(';');
        if (semi != std::string::npos) line = line.substr(0, semi);
        clean += line + "\n";
    }
    size_t i = 0;
    while (i < clean.size()) {
        if (std::isspace(static_cast<unsigned char>(clean[i]))) {
            ++i;
            continue;
        }
        if (clean[i] == '(' || clean[i] == ')') {
            toks.emplace_back(1, clean[i]);
            ++i;
            continue;
        }
        size_t j = i;
        while (j < clean.size() && !std::isspace(static_cast<unsigned char>(clean[j])) &&
               clean[j] != '(' && clean[j] != ')') {
            ++j;
        }
        toks.push_back(clean.substr(i, j - i));
        i = j;
    }
    return toks;
}

SExpr lisp_parse_one(const std::vector<std::string>& toks, size_t& pos) {
    if (pos >= toks.size()) return SExpr{};
    const std::string& t = toks[pos++];
    if (t == "(") {
        SExpr node;
        node.kind = SExpr::Kind::LIST;
        while (pos < toks.size() && toks[pos] != ")") {
            node.list.push_back(lisp_parse_one(toks, pos));
        }
        if (pos < toks.size() && toks[pos] == ")") ++pos;
        return node;
    }
    if (is_numeric_tok(t)) {
        SExpr node;
        node.kind = SExpr::Kind::NUMBER;
        node.num = std::stoll(t);
        return node;
    }
    SExpr node;
    node.kind = SExpr::Kind::SYMBOL;
    node.sym = to_lower_copy(t);
    return node;
}

struct LispVal {
    bool is_list = false;
    int64_t num = 0;
    std::string str;
    std::vector<int64_t> list;
};

struct LispDefun {
    std::vector<std::string> params;
    SExpr body;
};

LispVal eval_lisp_expr(
    const SExpr& expr,
    std::unordered_map<std::string, LispVal>& env,
    const std::unordered_map<std::string, LispDefun>& funcs,
    std::string& out_stream,
    size_t& steps)
{
    ++steps;
    if (steps > 50000) return {};

    if (expr.kind == SExpr::Kind::NUMBER) {
        LispVal v;
        v.num = expr.num;
        return v;
    }
    if (expr.kind == SExpr::Kind::SYMBOL) {
        if (expr.sym == "nil") {
            LispVal v;
            v.is_list = true;
            return v;
        }
        auto it = env.find(expr.sym);
        if (it != env.end()) return it->second;
        return {};
    }
    if (expr.list.empty()) {
        LispVal v;
        v.is_list = true;
        return v;
    }

    const std::string& op = expr.list[0].sym;
    if (op == "defun") return {};
    if (op == "let") {
        auto saved_env = env;
        if (expr.list.size() >= 2) {
            for (const auto& bind : expr.list[1].list) {
                if (bind.kind == SExpr::Kind::LIST && bind.list.size() >= 2) {
                    env[bind.list[0].sym] = eval_lisp_expr(bind.list[1], saved_env, funcs, out_stream, steps);
                }
            }
        }
        LispVal res;
        for (size_t i = 2; i < expr.list.size(); ++i) {
            res = eval_lisp_expr(expr.list[i], env, funcs, out_stream, steps);
        }
        env = std::move(saved_env);
        return res;
    }
    if (op == "if") {
        LispVal cond = eval_lisp_expr(expr.list[1], env, funcs, out_stream, steps);
        bool truth = cond.is_list ? !cond.list.empty() : (cond.num != 0);
        if (truth && expr.list.size() > 2) {
            return eval_lisp_expr(expr.list[2], env, funcs, out_stream, steps);
        }
        if (!truth && expr.list.size() > 3) {
            return eval_lisp_expr(expr.list[3], env, funcs, out_stream, steps);
        }
        return {};
    }
    if (op == "progn") {
        LispVal res;
        for (size_t i = 1; i < expr.list.size(); ++i) {
            res = eval_lisp_expr(expr.list[i], env, funcs, out_stream, steps);
        }
        return res;
    }
    if (op == "list") {
        LispVal res;
        res.is_list = true;
        for (size_t i = 1; i < expr.list.size(); ++i) {
            res.list.push_back(eval_lisp_expr(expr.list[i], env, funcs, out_stream, steps).num);
        }
        return res;
    }
    if (op == "null") {
        LispVal v = eval_lisp_expr(expr.list[1], env, funcs, out_stream, steps);
        LispVal r;
        r.num = (v.is_list && v.list.empty()) ? 1 : 0;
        return r;
    }
    if (op == "car") {
        LispVal v = eval_lisp_expr(expr.list[1], env, funcs, out_stream, steps);
        LispVal r;
        r.num = v.list.empty() ? 0 : v.list.front();
        return r;
    }
    if (op == "cdr") {
        LispVal v = eval_lisp_expr(expr.list[1], env, funcs, out_stream, steps);
        LispVal r;
        r.is_list = true;
        if (v.list.size() > 1) {
            r.list.assign(v.list.begin() + 1, v.list.end());
        }
        return r;
    }
    if (op == "code-char") {
        LispVal v = eval_lisp_expr(expr.list[1], env, funcs, out_stream, steps);
        LispVal r;
        r.num = v.num;
        r.str = std::string(1, static_cast<char>(v.num & 0xFF));
        return r;
    }
    if (op == "princ") {
        LispVal v = eval_lisp_expr(expr.list[1], env, funcs, out_stream, steps);
        if (!v.str.empty()) out_stream += v.str;
        else out_stream.push_back(static_cast<char>(v.num & 0xFF));
        return v;
    }
    if (op == "+" || op == "*" || op == "-" || op == "logxor") {
        std::vector<int64_t> args;
        for (size_t i = 1; i < expr.list.size(); ++i) {
            args.push_back(eval_lisp_expr(expr.list[i], env, funcs, out_stream, steps).num);
        }
        LispVal r;
        if (args.empty()) return r;
        int64_t acc = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            if (op == "+") acc += args[i];
            else if (op == "*") acc *= args[i];
            else if (op == "-") acc -= args[i];
            else if (op == "logxor") acc ^= args[i];
        }
        r.num = acc;
        return r;
    }
    if (op == "=" || op == ">=" || op == "<=") {
        int64_t a = eval_lisp_expr(expr.list[1], env, funcs, out_stream, steps).num;
        int64_t b = eval_lisp_expr(expr.list[2], env, funcs, out_stream, steps).num;
        LispVal r;
        if (op == "=") r.num = (a == b) ? 1 : 0;
        else if (op == ">=") r.num = (a >= b) ? 1 : 0;
        else r.num = (a <= b) ? 1 : 0;
        return r;
    }

    auto fit = funcs.find(op);
    if (fit != funcs.end()) {
        std::unordered_map<std::string, LispVal> sub_env;
        for (size_t i = 0; i < fit->second.params.size() && (i + 1) < expr.list.size(); ++i) {
            sub_env[fit->second.params[i]] = eval_lisp_expr(expr.list[i + 1], env, funcs, out_stream, steps);
        }
        return eval_lisp_expr(fit->second.body, sub_env, funcs, out_stream, steps);
    }

    return {};
}

// ============================================================================
// 5. Prolog Parser & Unification Engine
// ============================================================================
struct PrologTerm {
    bool is_var = false;
    bool is_int = false;
    int64_t val = 0;
    std::string name;
};

struct PrologGoal {
    std::string pred;
    std::string op; // "call", "is", ">=", "=<", ">"
    std::vector<PrologTerm> args;
    std::string raw_rhs;
};

struct PrologClause {
    std::string head_pred;
    std::vector<PrologTerm> head_args;
    std::vector<PrologGoal> body;
};

PrologTerm make_prolog_term(const std::string& raw) {
    std::string t = trim_copy(raw);
    PrologTerm term;
    if (!t.empty() && (std::isupper(static_cast<unsigned char>(t[0])) || t[0] == '_')) {
        term.is_var = true;
        term.name = t;
    } else if (is_numeric_tok(t)) {
        term.is_int = true;
        term.val = std::stoll(t);
    } else {
        term.name = t;
    }
    return term;
}

std::vector<std::string> split_top_commas(const std::string& s) {
    std::vector<std::string> parts;
    int depth = 0;
    size_t start = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '(') ++depth;
        else if (s[i] == ')') --depth;
        else if (s[i] == ',' && depth == 0) {
            parts.push_back(trim_copy(s.substr(start, i - start)));
            start = i + 1;
        }
    }
    if (start <= s.size()) {
        std::string last = trim_copy(s.substr(start));
        if (!last.empty()) parts.push_back(last);
    }
    return parts;
}

int64_t eval_prolog_arith(const std::string& expr, const std::unordered_map<std::string, int64_t>& env) {
    std::string s = expr;
    std::vector<std::string> toks;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) toks.push_back(tok);
    if (toks.empty()) return 0;

    auto resolve = [&](const std::string& tk) -> int64_t {
        if (is_numeric_tok(tk)) return std::stoll(tk);
        auto it = env.find(tk);
        return (it != env.end()) ? it->second : 0;
    };

    // Pass 1: handle '*'
    std::vector<int64_t> nums;
    std::vector<std::string> ops;
    nums.push_back(resolve(toks[0]));
    for (size_t i = 1; i + 1 < toks.size(); i += 2) {
        std::string op = toks[i];
        int64_t rhs = resolve(toks[i + 1]);
        if (op == "*") {
            nums.back() *= rhs;
        } else {
            ops.push_back(op);
            nums.push_back(rhs);
        }
    }
    int64_t res = nums[0];
    for (size_t i = 0; i < ops.size(); ++i) {
        if (ops[i] == "+") res += nums[i + 1];
        else if (ops[i] == "-") res -= nums[i + 1];
        else if (ops[i] == "xor") res ^= nums[i + 1];
    }
    return res;
}

bool prove_prolog_goals(
    const std::vector<PrologClause>& db,
    const std::vector<PrologGoal>& goals,
    size_t idx,
    std::unordered_map<std::string, int64_t>& env,
    std::string& out,
    size_t& steps)
{
    if (steps > 40000) return false;
    if (idx >= goals.size()) return true;
    ++steps;

    const PrologGoal& g = goals[idx];
    auto resolve_val = [&](const PrologTerm& t, int64_t& out_v) -> bool {
        if (t.is_int) { out_v = t.val; return true; }
        if (t.is_var) {
            auto it = env.find(t.name);
            if (it == env.end()) return false;
            out_v = it->second;
            return true;
        }
        return false;
    };

    if (g.op == "is") {
        int64_t v = eval_prolog_arith(g.raw_rhs, env);
        auto saved = env;
        env[g.args[0].name] = v;
        if (prove_prolog_goals(db, goals, idx + 1, env, out, steps)) return true;
        env = std::move(saved);
        return false;
    }
    if (g.op == ">=" || g.op == "=<" || g.op == ">") {
        int64_t a = 0, b = 0;
        if (!resolve_val(g.args[0], a) || !resolve_val(g.args[1], b)) return false;
        bool ok = (g.op == ">=") ? (a >= b) : (g.op == "=<") ? (a <= b) : (a > b);
        if (!ok) return false;
        return prove_prolog_goals(db, goals, idx + 1, env, out, steps);
    }
    if (g.pred == "put_code") {
        int64_t c = 0;
        if (!resolve_val(g.args[0], c)) return false;
        out.push_back(static_cast<char>(c & 0xFF));
        return prove_prolog_goals(db, goals, idx + 1, env, out, steps);
    }

    for (const auto& cl : db) {
        if (cl.head_pred != g.pred || cl.head_args.size() != g.args.size()) continue;
        std::unordered_map<std::string, int64_t> trial_env = env;
        std::unordered_map<std::string, int64_t> sub_env;
        std::vector<std::pair<std::string, std::string>> var_out_links;
        bool match = true;
        for (size_t k = 0; k < g.args.size(); ++k) {
            int64_t caller_val = 0;
            auto it_t = g.args[k].is_var ? trial_env.find(g.args[k].name) : trial_env.end();
            bool caller_bound = g.args[k].is_int || (g.args[k].is_var && it_t != trial_env.end());
            if (g.args[k].is_int) caller_val = g.args[k].val;
            else if (caller_bound) caller_val = it_t->second;

            if (cl.head_args[k].is_int) {
                if (caller_bound) {
                    if (caller_val != cl.head_args[k].val) { match = false; break; }
                } else if (g.args[k].is_var) {
                    trial_env[g.args[k].name] = cl.head_args[k].val;
                } else {
                    match = false; break;
                }
            } else if (cl.head_args[k].is_var) {
                if (caller_bound) {
                    auto it = sub_env.find(cl.head_args[k].name);
                    if (it != sub_env.end() && it->second != caller_val) { match = false; break; }
                    sub_env[cl.head_args[k].name] = caller_val;
                } else if (g.args[k].is_var) {
                    var_out_links.push_back({ g.args[k].name, cl.head_args[k].name });
                }
            }
        }
        if (!match) continue;

        std::string out_checkpoint = out;
        if (prove_prolog_goals(db, cl.body, 0, sub_env, out, steps)) {
            auto saved_env = env;
            env = std::move(trial_env);
            bool ok_bind = true;
            for (const auto& lk : var_out_links) {
                auto sit = sub_env.find(lk.second);
                if (sit != sub_env.end()) {
                    env[lk.first] = sit->second;
                } else {
                    ok_bind = false;
                }
            }
            if (ok_bind && prove_prolog_goals(db, goals, idx + 1, env, out, steps)) {
                return true;
            }
            env = std::move(saved_env);
        }
        out = out_checkpoint;
    }
    return false;
}

} // namespace

// ============================================================================
// Public Engine 1: x86-64 Assembly
// ============================================================================
PolyglotExecResult run_x86_64_asm(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Assembly (x86-64)";
    res.filename = fname;
    res.source_bytes = src.size();

    std::vector<AsmInsn> code;
    std::unordered_map<std::string, size_t> labels;
    std::unordered_map<std::string, int64_t> symbols;
    symbols["out_buf"] = 0x1000;
    symbols["checksum_acc"] = 0x1100;

    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        auto sc = line.find(';');
        if (sc != std::string::npos) line = line.substr(0, sc);
        line = trim_copy(line);
        if (line.empty()) continue;
        std::string low = to_lower_copy(line);
        if (low.rfind("section ", 0) == 0 || low.rfind("global ", 0) == 0) continue;
        if (low.find(" equ ") != std::string::npos || low.find(" resb ") != std::string::npos || low.find(" resq ") != std::string::npos) {
            continue;
        }
        if (line.back() == ':') {
            labels[to_lower_copy(line.substr(0, line.size() - 1))] = code.size();
            continue;
        }
        size_t sp = line.find_first_of(" \t");
        AsmInsn insn;
        if (sp == std::string::npos) {
            insn.op = to_lower_copy(line);
        } else {
            insn.op = to_lower_copy(line.substr(0, sp));
            std::string rest = trim_copy(line.substr(sp + 1));
            auto comma = rest.find(',');
            if (comma == std::string::npos) {
                insn.arg1 = trim_copy(rest);
            } else {
                insn.arg1 = trim_copy(rest.substr(0, comma));
                insn.arg2 = trim_copy(rest.substr(comma + 1));
            }
        }
        code.push_back(insn);
    }

    std::unordered_map<std::string, int64_t> regs;
    for (const char* r : {"rax","rbx","rcx","rdx","rsi","rdi","rbp","rsp","r8","r9","r10","r11","r12","r13","r14","r15"}) {
        regs[r] = 0;
    }
    regs["rsp"] = 0xE000;
    std::vector<uint8_t> mem(65536, 0);
    std::vector<size_t> call_stack;
    bool zf = false, sf = false;

    auto norm_reg = [](std::string r) -> std::string {
        r = to_lower_copy(trim_copy(r));
        if (r == "al" || r == "eax") return "rax";
        return r;
    };

    auto read_mem64 = [&](int64_t addr) -> int64_t {
        if (addr < 0 || addr + 7 >= static_cast<int64_t>(mem.size())) return 0;
        uint64_t v = 0;
        for (int b = 0; b < 8; ++b) {
            v |= (static_cast<uint64_t>(mem[static_cast<size_t>(addr + b)]) << (b * 8));
        }
        return static_cast<int64_t>(v);
    };
    auto write_mem64 = [&](int64_t addr, int64_t val) {
        if (addr < 0 || addr + 7 >= static_cast<int64_t>(mem.size())) return;
        uint64_t v = static_cast<uint64_t>(val);
        for (int b = 0; b < 8; ++b) {
            mem[static_cast<size_t>(addr + b)] = static_cast<uint8_t>((v >> (b * 8)) & 0xFF);
        }
    };

    auto eval_op = [&](const std::string& arg) -> int64_t {
        std::string t = trim_copy(arg);
        if (t.size() >= 2 && t.front() == '[' && t.back() == ']') {
            std::string inner = to_lower_copy(trim_copy(t.substr(1, t.size() - 2)));
            if (symbols.count(inner)) return read_mem64(symbols[inner]);
            if (regs.count(inner)) return read_mem64(regs[inner]);
            return 0;
        }
        std::string nr = norm_reg(t);
        if (regs.count(nr)) {
            if (to_lower_copy(t) == "al") return regs[nr] & 0xFF;
            return regs[nr];
        }
        if (symbols.count(to_lower_copy(t))) return symbols[to_lower_copy(t)];
        return parse_imm(t);
    };

    size_t pc = labels.count("_start") ? labels["_start"] : 0;
    std::string out_buf;
    bool halted = false;
    while (pc < code.size() && res.steps_executed < 50000 && !halted) {
        ++res.steps_executed;
        const AsmInsn& in = code[pc++];
        if (in.op == "mov") {
            int64_t val = eval_op(in.arg2);
            if (in.arg1.size() >= 2 && in.arg1.front() == '[' && in.arg1.back() == ']') {
                std::string inner = to_lower_copy(trim_copy(in.arg1.substr(1, in.arg1.size() - 2)));
                int64_t addr = symbols.count(inner) ? symbols[inner] : regs[norm_reg(inner)];
                if (to_lower_copy(in.arg2) == "al") {
                    if (addr >= 0 && addr < static_cast<int64_t>(mem.size())) {
                        mem[static_cast<size_t>(addr)] = static_cast<uint8_t>(val & 0xFF);
                    }
                } else {
                    write_mem64(addr, val);
                }
            } else {
                regs[norm_reg(in.arg1)] = val;
            }
        } else if (in.op == "lea") {
            std::string inner = trim_copy(in.arg2);
            if (inner.size() >= 2 && inner.front() == '[') inner = trim_copy(inner.substr(1, inner.size() - 2));
            std::string low_in = to_lower_copy(inner);
            regs[norm_reg(in.arg1)] = symbols.count(low_in) ? symbols[low_in] : 0;
        } else if (in.op == "add") {
            std::string r = norm_reg(in.arg1);
            regs[r] += eval_op(in.arg2);
        } else if (in.op == "sub") {
            std::string r = norm_reg(in.arg1);
            regs[r] -= eval_op(in.arg2);
        } else if (in.op == "imul") {
            std::string r = norm_reg(in.arg1);
            regs[r] *= eval_op(in.arg2);
        } else if (in.op == "xor") {
            std::string r = norm_reg(in.arg1);
            regs[r] ^= eval_op(in.arg2);
        } else if (in.op == "and") {
            std::string r = norm_reg(in.arg1);
            regs[r] &= eval_op(in.arg2);
        } else if (in.op == "shl") {
            std::string r = norm_reg(in.arg1);
            regs[r] <<= eval_op(in.arg2);
        } else if (in.op == "shr") {
            std::string r = norm_reg(in.arg1);
            regs[r] = static_cast<int64_t>(static_cast<uint64_t>(regs[r]) >> eval_op(in.arg2));
        } else if (in.op == "inc") {
            regs[norm_reg(in.arg1)]++;
        } else if (in.op == "dec") {
            regs[norm_reg(in.arg1)]--;
        } else if (in.op == "push") {
            regs["rsp"] -= 8;
            write_mem64(regs["rsp"], eval_op(in.arg1));
        } else if (in.op == "pop") {
            regs[norm_reg(in.arg1)] = read_mem64(regs["rsp"]);
            regs["rsp"] += 8;
        } else if (in.op == "cmp") {
            int64_t diff = eval_op(in.arg1) - eval_op(in.arg2);
            zf = (diff == 0);
            sf = (diff < 0);
        } else if (in.op == "jmp") {
            std::string lbl = to_lower_copy(in.arg1);
            if (labels.count(lbl)) pc = labels[lbl];
        } else if (in.op == "jne") {
            if (!zf && labels.count(to_lower_copy(in.arg1))) pc = labels[to_lower_copy(in.arg1)];
        } else if (in.op == "je") {
            if (zf && labels.count(to_lower_copy(in.arg1))) pc = labels[to_lower_copy(in.arg1)];
        } else if (in.op == "jl") {
            if (sf && labels.count(to_lower_copy(in.arg1))) pc = labels[to_lower_copy(in.arg1)];
        } else if (in.op == "call") {
            std::string lbl = to_lower_copy(in.arg1);
            if (labels.count(lbl)) {
                call_stack.push_back(pc);
                pc = labels[lbl];
            }
        } else if (in.op == "ret") {
            if (!call_stack.empty()) {
                pc = call_stack.back();
                call_stack.pop_back();
            } else {
                halted = true;
            }
        } else if (in.op == "syscall") {
            if (regs["rax"] == 1 && regs["rdi"] == 1) {
                int64_t addr = regs["rsi"];
                int64_t len = regs["rdx"];
                for (int64_t i = 0; i < len && addr + i < static_cast<int64_t>(mem.size()); ++i) {
                    out_buf.push_back(static_cast<char>(mem[static_cast<size_t>(addr + i)]));
                }
            } else if (regs["rax"] == 60) {
                halted = true;
            }
        }
    }

    res.output = out_buf;
    res.checksum = regs["r14"];
    res.verified = (res.output == "Hello world" && res.checksum == 1084);
    std::ostringstream ts;
    ts << "x86-64 EMU | steps=" << res.steps_executed
       << " | r14_sum=" << regs["r14"] << " r15=" << regs["r15"]
       << " | sys_write(1,0x" << std::hex << regs["rsi"] << std::dec << "," << regs["rdx"]
       << ") -> \"" << res.output << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// Public Engine 2: Befunge-93 2D Esoteric Grid VM
// ============================================================================
PolyglotExecResult run_befunge_93(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Befunge-93 (2D Grid)";
    res.filename = fname;
    res.source_bytes = src.size();

    std::vector<std::string> grid;
    std::istringstream iss(src);
    std::string line;
    size_t max_w = 0;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        max_w = std::max(max_w, line.size());
        grid.push_back(line);
    }
    if (grid.empty() || max_w == 0) return res;
    for (auto& row : grid) row.resize(max_w, ' ');

    int H = static_cast<int>(grid.size());
    int W = static_cast<int>(max_w);
    int x = 0, y = 0, dx = 1, dy = 0;
    bool str_mode = false;
    std::vector<int64_t> st;
    auto pop = [&]() -> int64_t {
        if (st.empty()) return 0;
        int64_t v = st.back();
        st.pop_back();
        return v;
    };

    std::string out;
    while (res.steps_executed < 50000) {
        ++res.steps_executed;
        char op = grid[static_cast<size_t>(y)][static_cast<size_t>(x)];
        if (str_mode) {
            if (op == '"') str_mode = false;
            else st.push_back(static_cast<unsigned char>(op));
        } else {
            if (op == '@') break;
            else if (op >= '0' && op <= '9') st.push_back(op - '0');
            else if (op == '"') str_mode = true;
            else if (op == '+') { int64_t a = pop(), b = pop(); st.push_back(b + a); }
            else if (op == '-') { int64_t a = pop(), b = pop(); st.push_back(b - a); }
            else if (op == '*') { int64_t a = pop(), b = pop(); st.push_back(b * a); }
            else if (op == '/') { int64_t a = pop(), b = pop(); st.push_back(a ? b / a : 0); }
            else if (op == '%') { int64_t a = pop(), b = pop(); st.push_back(a ? b % a : 0); }
            else if (op == '!') { int64_t a = pop(); st.push_back(a == 0 ? 1 : 0); }
            else if (op == '`') { int64_t a = pop(), b = pop(); st.push_back(b > a ? 1 : 0); }
            else if (op == '>') { dx = 1; dy = 0; }
            else if (op == '<') { dx = -1; dy = 0; }
            else if (op == '^') { dx = 0; dy = -1; }
            else if (op == 'v') { dx = 0; dy = 1; }
            else if (op == '_') { int64_t a = pop(); dx = (a == 0) ? 1 : -1; dy = 0; }
            else if (op == '|') { int64_t a = pop(); dx = 0; dy = (a == 0) ? 1 : -1; }
            else if (op == ':') { int64_t a = pop(); st.push_back(a); st.push_back(a); }
            else if (op == '\\') { int64_t a = pop(), b = pop(); st.push_back(a); st.push_back(b); }
            else if (op == '$') { pop(); }
            else if (op == '.') { out += std::to_string(pop()); }
            else if (op == ',') { out.push_back(static_cast<char>(pop() & 0xFF)); }
            else if (op == '#') { x = (x + dx + W) % W; y = (y + dy + H) % H; }
        }
        x = (x + dx + W) % W;
        y = (y + dy + H) % H;
    }

    res.output = out;
    res.checksum = calc_ascii_sum(out);
    res.verified = (out == "Hello world");
    std::ostringstream ts;
    ts << "Befunge-93 2D VM (" << W << "x" << H << " grid) | steps=" << res.steps_executed
       << " | final_IP=(" << x << "," << y << ") | out=\"" << out << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// Public Engine 3: Forth Stack Dictionary VM
// ============================================================================
PolyglotExecResult run_forth_vm(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Forth";
    res.filename = fname;
    res.source_bytes = src.size();

    // Strip '\' line comments and '( ... )' paren comments
    std::string cleaned;
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        auto bs = line.find('\\');
        if (bs != std::string::npos) line = line.substr(0, bs);
        std::string no_paren;
        bool in_p = false;
        for (char c : line) {
            if (c == '(') in_p = true;
            else if (c == ')') in_p = false;
            else if (!in_p) no_paren.push_back(c);
        }
        cleaned += no_paren + "\n";
    }

    std::vector<std::string> tokens;
    std::istringstream tiss(cleaned);
    std::string tok;
    while (tiss >> tok) tokens.push_back(tok);

    std::unordered_map<std::string, std::vector<std::string>> dict;
    std::unordered_map<std::string, int64_t> constants;
    std::unordered_map<std::string, size_t> var_slots;
    std::vector<int64_t> mem;
    std::vector<int64_t> ds;
    std::string out;

    auto pop = [&]() -> int64_t {
        if (ds.empty()) return 0;
        int64_t v = ds.back();
        ds.pop_back();
        return v;
    };

    std::function<void(const std::vector<std::string>&, int64_t)> exec_seq;
    exec_seq = [&](const std::vector<std::string>& seq, int64_t loop_idx) {
        for (size_t i = 0; i < seq.size() && res.steps_executed < 50000; ++i) {
            ++res.steps_executed;
            const std::string& w = seq[i];
            if (is_numeric_tok(w)) {
                ds.push_back(std::stoll(w));
            } else if (w == "+") { int64_t a = pop(), b = pop(); ds.push_back(b + a); }
            else if (w == "-") { int64_t a = pop(), b = pop(); ds.push_back(b - a); }
            else if (w == "*") { int64_t a = pop(), b = pop(); ds.push_back(b * a); }
            else if (w == "/") { int64_t a = pop(), b = pop(); ds.push_back(a ? b / a : 0); }
            else if (w == "MOD") { int64_t a = pop(), b = pop(); ds.push_back(a ? b % a : 0); }
            else if (w == "XOR") { int64_t a = pop(), b = pop(); ds.push_back(b ^ a); }
            else if (w == "DUP") { int64_t a = pop(); ds.push_back(a); ds.push_back(a); }
            else if (w == "DROP") { pop(); }
            else if (w == "SWAP") { int64_t a = pop(), b = pop(); ds.push_back(a); ds.push_back(b); }
            else if (w == "OVER") { int64_t a = pop(), b = pop(); ds.push_back(b); ds.push_back(a); ds.push_back(b); }
            else if (w == "ROT") { int64_t c = pop(), b = pop(), a = pop(); ds.push_back(b); ds.push_back(c); ds.push_back(a); }
            else if (w == "EMIT") { out.push_back(static_cast<char>(pop() & 0xFF)); }
            else if (w == "CR") { /* ignore trailing newline */ }
            else if (w == ".") { pop(); }
            else if (w == "@") {
                size_t slot = static_cast<size_t>(pop());
                ds.push_back(slot < mem.size() ? mem[slot] : 0);
            } else if (w == "!") {
                size_t slot = static_cast<size_t>(pop());
                int64_t val = pop();
                if (slot < mem.size()) mem[slot] = val;
            } else if (w == "CONSTANT" && i + 1 < seq.size()) {
                constants[seq[++i]] = pop();
            } else if (w == "VARIABLE" && i + 1 < seq.size()) {
                var_slots[seq[++i]] = mem.size();
                mem.push_back(0);
            } else if (w == "DO") {
                int64_t start_i = pop();
                int64_t limit = pop();
                size_t end_i = i + 1;
                while (end_i < seq.size() && seq[end_i] != "LOOP") ++end_i;
                std::vector<std::string> body(seq.begin() + i + 1, seq.begin() + end_i);
                for (int64_t k = start_i; k < limit; ++k) {
                    exec_seq(body, k);
                }
                i = end_i;
            } else if (w == "I") {
                ds.push_back(loop_idx);
            } else if (constants.count(w)) {
                ds.push_back(constants[w]);
            } else if (var_slots.count(w)) {
                ds.push_back(static_cast<int64_t>(var_slots[w]));
            } else if (dict.count(w)) {
                exec_seq(dict[w], loop_idx);
            }
        }
    };

    std::vector<std::string> top_seq;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == ":" && i + 1 < tokens.size()) {
            std::string name = tokens[++i];
            std::vector<std::string> body;
            ++i;
            while (i < tokens.size() && tokens[i] != ";") {
                body.push_back(tokens[i++]);
            }
            dict[name] = std::move(body);
        } else {
            top_seq.push_back(tokens[i]);
        }
    }
    exec_seq(top_seq, 0);

    res.output = out;
    res.checksum = var_slots.count("CHECKSUM") ? mem[var_slots["CHECKSUM"]] : calc_ascii_sum(out);
    res.verified = (out == "Hello world" && res.checksum == 1084);
    std::ostringstream ts;
    ts << "Forth Dict VM (" << dict.size() << " words) | steps=" << res.steps_executed
       << " | CHECKSUM=" << res.checksum << " | EMIT -> \"" << out << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// Public Engine 4: Common Lisp S-Expr Evaluator
// ============================================================================
PolyglotExecResult run_lisp_eval(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Common Lisp";
    res.filename = fname;
    res.source_bytes = src.size();

    auto toks = lisp_tokenize(src);
    size_t pos = 0;
    std::unordered_map<std::string, LispDefun> funcs;
    std::unordered_map<std::string, LispVal> global_env;
    std::string out_stream;

    while (pos < toks.size()) {
        SExpr node = lisp_parse_one(toks, pos);
        if (node.kind == SExpr::Kind::LIST && !node.list.empty() && node.list[0].sym == "defun" && node.list.size() >= 4) {
            LispDefun fn;
            for (const auto& p : node.list[2].list) fn.params.push_back(p.sym);
            fn.body = node.list[3];
            funcs[node.list[1].sym] = std::move(fn);
        } else {
            eval_lisp_expr(node, global_env, funcs, out_stream, res.steps_executed);
        }
    }

    res.output = out_stream;
    res.checksum = calc_ascii_sum(out_stream);
    res.verified = (out_stream == "Hello world");
    std::ostringstream ts;
    ts << "Lisp AST Evaluator (" << funcs.size() << " defuns) | evals=" << res.steps_executed
       << " | sum=" << res.checksum << " -> \"" << out_stream << "\"";
    res.trace_summary = ts.str();
    return res;
}

// ============================================================================
// Public Engine 5: Prolog Horn-Clause Unification Engine
// ============================================================================
PolyglotExecResult run_prolog_solver(const std::string& src, const std::string& fname) {
    PolyglotExecResult res;
    res.language = "Prolog";
    res.filename = fname;
    res.source_bytes = src.size();

    std::string clean;
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        auto pct = line.find('%');
        if (pct != std::string::npos) line = line.substr(0, pct);
        clean += line + " ";
    }

    std::vector<PrologClause> db;
    std::vector<PrologGoal> queries;

    auto parse_atom_goal = [&](const std::string& txt) -> PrologGoal {
        PrologGoal g;
        std::string s = trim_copy(txt);
        auto check_infix = [&](const std::string& op_tok) -> bool {
            size_t p = s.find(op_tok);
            if (p == std::string::npos) return false;
            g.op = trim_copy(op_tok);
            g.args.push_back(make_prolog_term(s.substr(0, p)));
            std::string rhs = trim_copy(s.substr(p + op_tok.size()));
            if (g.op == "is") g.raw_rhs = rhs;
            else g.args.push_back(make_prolog_term(rhs));
            return true;
        };
        if (check_infix(" is ") || check_infix(" >= ") || check_infix(" =< ") || check_infix(" > ")) {
            return g;
        }
        g.op = "call";
        auto lp = s.find('(');
        if (lp == std::string::npos) {
            g.pred = s;
        } else {
            g.pred = trim_copy(s.substr(0, lp));
            auto rp = s.rfind(')');
            if (rp > lp) {
                for (const auto& a : split_top_commas(s.substr(lp + 1, rp - lp - 1))) {
                    g.args.push_back(make_prolog_term(a));
                }
            }
        }
        return g;
    };

    size_t start = 0;
    while (start < clean.size()) {
        auto dot = clean.find('.', start);
        if (dot == std::string::npos) break;
        std::string stmt = trim_copy(clean.substr(start, dot - start));
        start = dot + 1;
        if (stmt.empty()) continue;

        if (stmt.rfind("?-", 0) == 0) {
            for (const auto& gs : split_top_commas(stmt.substr(2))) {
                queries.push_back(parse_atom_goal(gs));
            }
            continue;
        }

        auto neck = stmt.find(":-");
        PrologClause cl;
        if (neck == std::string::npos) {
            PrologGoal head = parse_atom_goal(stmt);
            cl.head_pred = head.pred;
            cl.head_args = std::move(head.args);
        } else {
            PrologGoal head = parse_atom_goal(stmt.substr(0, neck));
            cl.head_pred = head.pred;
            cl.head_args = std::move(head.args);
            for (const auto& gs : split_top_commas(stmt.substr(neck + 2))) {
                cl.body.push_back(parse_atom_goal(gs));
            }
        }
        db.push_back(std::move(cl));
    }

    std::unordered_map<std::string, int64_t> env;
    std::string out;
    bool ok = prove_prolog_goals(db, queries, 0, env, out, res.steps_executed);
    res.output = out;
    res.checksum = calc_ascii_sum(out);
    res.verified = ok && (out == "Hello world");
    std::ostringstream ts;
    ts << "Prolog SLD Resolution (" << db.size() << " clauses) | unifications=" << res.steps_executed
       << " | ?- solve_hello -> \"" << out << "\"";
    res.trace_summary = ts.str();
    return res;
}

} // namespace govno::polyglot
