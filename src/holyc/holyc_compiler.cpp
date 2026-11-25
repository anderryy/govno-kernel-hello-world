#include "holyc/holyc_compiler.hpp"

#include <intrin.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace govno::holyc {

namespace {

std::string unescape_c_str(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char nxt = s[i + 1];
            if (nxt == 'n') { out.push_back('\n'); ++i; continue; }
            if (nxt == 't') { out.push_back('\t'); ++i; continue; }
            if (nxt == '"') { out.push_back('"'); ++i; continue; }
            if (nxt == '\\') { out.push_back('\\'); ++i; continue; }
        }
        out.push_back(s[i]);
    }
    return out;
}

std::string run_holyc_compile_time_exe_blocks(const std::string& raw_src, std::string& out_exe_log) {
    std::string result = raw_src;
    std::ostringstream log;
    log << "; === HOLYC #exe { ... } COMPILE-TIME STREAM EVALUATOR ===\r\n";

    size_t pos = 0;
    int block_id = 0;
    while ((pos = result.find("#exe", pos)) != std::string::npos) {
        size_t brace_open = result.find('{', pos);
        if (brace_open == std::string::npos) break;

        int depth = 1;
        size_t p = brace_open + 1;
        bool in_str = false;
        while (p < result.size() && depth > 0) {
            if (result[p] == '"' && (p == 0 || result[p - 1] != '\\')) in_str = !in_str;
            else if (!in_str) {
                if (result[p] == '{') ++depth;
                else if (result[p] == '}') --depth;
            }
            ++p;
        }
        if (depth != 0) break;

        std::string exe_body = result.substr(brace_open + 1, (p - 1) - (brace_open + 1));
        std::ostringstream generated_stream;
        size_t sp_pos = 0;
        while ((sp_pos = exe_body.find("StreamPrint", sp_pos)) != std::string::npos) {
            size_t q1 = exe_body.find('"', sp_pos);
            if (q1 == std::string::npos) break;
            size_t q2 = q1 + 1;
            while (q2 < exe_body.size()) {
                if (exe_body[q2] == '"' && exe_body[q2 - 1] != '\\') break;
                ++q2;
            }
            if (q2 < exe_body.size()) {
                generated_stream << unescape_c_str(exe_body.substr(q1 + 1, q2 - q1 - 1));
                sp_pos = q2 + 1;
            } else break;
        }

        std::string gen_code = generated_stream.str();
        log << "[#exe #" << block_id++ << "] Generated " << gen_code.size() << " bytes of HolyC at compile time:\r\n"
            << gen_code << "\r\n";
        result.replace(pos, p - pos, gen_code);
        pos += gen_code.size();
    }

    out_exe_log = log.str();
    return result;
}

enum class HCTok {
    EOF_TOK = 0,
    KW_U0, KW_I64, KW_U8, KW_F64, KW_CLASS,
    KW_IF, KW_ELSE, KW_WHILE, KW_RETURN, KW_ASM, KW_GODTALK,
    IDENT, NUMBER, STRING,
    ASSIGN, PLUS, MINUS, MUL, DIV, BACKTICK_POW,
    LE, LT, GE, GT, EQ, NE,
    LPAREN, RPAREN, LBRACE, RBRACE, SEMI, COMMA, DOT
};

struct HToken {
    HCTok type;
    std::string text;
    int line = 1;
};

std::vector<HToken> lex_holyc(const std::string& src) {
    std::vector<HToken> toks;
    size_t i = 0, n = src.size();
    int line = 1;

    while (i < n) {
        char c = src[i];
        if (c == ' ' || c == '\t' || c == '\r') { ++i; continue; }
        if (c == '\n') { ++i; ++line; continue; }
        if (c == '/' && i + 1 < n && src[i + 1] == '/') {
            while (i < n && src[i] != '\n') ++i;
            continue;
        }
        if (c == '/' && i + 1 < n && src[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(src[i] == '*' && src[i + 1] == '/')) {
                if (src[i] == '\n') ++line;
                ++i;
            }
            if (i + 1 < n) i += 2;
            continue;
        }
        if (c == '$') {
            ++i;
            while (i < n && src[i] != '$') ++i;
            if (i < n) ++i;
            continue;
        }
        if (c == '"') {
            ++i;
            std::string raw;
            while (i < n && !(src[i] == '"' && src[i - 1] != '\\')) raw.push_back(src[i++]);
            if (i < n) ++i;
            toks.push_back({HCTok::STRING, unescape_c_str(raw), line});
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            std::string num;
            while (i < n && std::isdigit(static_cast<unsigned char>(src[i]))) num.push_back(src[i++]);
            toks.push_back({HCTok::NUMBER, num, line});
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string id;
            while (i < n && (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_')) id.push_back(src[i++]);
            HCTok kw = HCTok::IDENT;
            if (id == "U0") kw = HCTok::KW_U0;
            else if (id == "I64") kw = HCTok::KW_I64;
            else if (id == "U8") kw = HCTok::KW_U8;
            else if (id == "F64") kw = HCTok::KW_F64;
            else if (id == "Class") kw = HCTok::KW_CLASS;
            else if (id == "if") kw = HCTok::KW_IF;
            else if (id == "else") kw = HCTok::KW_ELSE;
            else if (id == "while") kw = HCTok::KW_WHILE;
            else if (id == "return") kw = HCTok::KW_RETURN;
            else if (id == "asm") kw = HCTok::KW_ASM;
            else if (id == "GodTalk") kw = HCTok::KW_GODTALK;
            toks.push_back({kw, id, line});
            continue;
        }

        if (i + 1 < n) {
            std::string two = src.substr(i, 2);
            if (two == "<=") { toks.push_back({HCTok::LE, two, line}); i += 2; continue; }
            if (two == ">=") { toks.push_back({HCTok::GE, two, line}); i += 2; continue; }
            if (two == "==") { toks.push_back({HCTok::EQ, two, line}); i += 2; continue; }
            if (two == "!=") { toks.push_back({HCTok::NE, two, line}); i += 2; continue; }
        }

        HCTok single = HCTok::EOF_TOK;
        switch (c) {
            case '=': single = HCTok::ASSIGN; break;
            case '+': single = HCTok::PLUS; break;
            case '-': single = HCTok::MINUS; break;
            case '*': single = HCTok::MUL; break;
            case '/': single = HCTok::DIV; break;
            case '`': single = HCTok::BACKTICK_POW; break;
            case '<': single = HCTok::LT; break;
            case '>': single = HCTok::GT; break;
            case '(': single = HCTok::LPAREN; break;
            case ')': single = HCTok::RPAREN; break;
            case '{': single = HCTok::LBRACE; break;
            case '}': single = HCTok::RBRACE; break;
            case ';': single = HCTok::SEMI; break;
            case ',': single = HCTok::COMMA; break;
            case '.': single = HCTok::DOT; break;
            default:
                throw std::runtime_error(std::string("HolyC Lexer error at line ") + std::to_string(line) + " on '" + c + "'");
        }
        toks.push_back({single, std::string(1, c), line});
        ++i;
    }
    toks.push_back({HCTok::EOF_TOK, "<EOF>", line});
    return toks;
}

struct HCExpr { virtual ~HCExpr() = default; };
struct HCNumExpr : HCExpr { int64_t val; explicit HCNumExpr(int64_t v) : val(v) {} };
struct HCVarExpr : HCExpr { std::string name; explicit HCVarExpr(std::string n) : name(std::move(n)) {} };
struct HCBinExpr : HCExpr {
    HCTok op; std::shared_ptr<HCExpr> lhs, rhs;
    HCBinExpr(HCTok o, std::shared_ptr<HCExpr> l, std::shared_ptr<HCExpr> r) : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
};
struct HCChainedCmpExpr : HCExpr { std::vector<std::shared_ptr<HCExpr>> operands; std::vector<HCTok> ops; };
struct HCCallExpr : HCExpr { std::string callee; std::vector<std::shared_ptr<HCExpr>> args; };

struct HCStmt { virtual ~HCStmt() = default; };
struct HCBlockStmt : HCStmt { std::vector<std::shared_ptr<HCStmt>> stmts; };
struct HCVarDeclStmt : HCStmt { std::string name; std::shared_ptr<HCExpr> init; };
struct HCAssignStmt : HCStmt { std::string name; std::shared_ptr<HCExpr> val; };
struct HCExprStmt : HCStmt { std::shared_ptr<HCExpr> expr; };
struct HCPrintStrStmt : HCStmt { std::string str; };
struct HCReturnStmt : HCStmt { std::shared_ptr<HCExpr> expr; };
struct HCIfStmt : HCStmt { std::shared_ptr<HCExpr> cond; std::shared_ptr<HCStmt> then_br, else_br; };
struct HCWhileStmt : HCStmt { std::shared_ptr<HCExpr> cond; std::shared_ptr<HCStmt> body; };
struct HCGodTalkStmt : HCStmt { std::shared_ptr<HCExpr> count_expr; };
struct HCInlineAsmStmt : HCStmt { std::string var_to_emit, asm_text; };

struct HCFuncDecl {
    std::string ret_type, name;
    std::vector<std::string> params;
    std::shared_ptr<HCBlockStmt> body;
};

struct HCProgram {
    std::unordered_set<std::string> class_names;
    std::vector<std::shared_ptr<HCVarDeclStmt>> globals;
    std::vector<HCFuncDecl> functions;
};

class HolyCParser {
    const std::vector<HToken>& toks_;
    size_t pos_ = 0;
    std::unordered_set<std::string> known_classes_;
    std::unordered_set<std::string> known_funcs_;

    const HToken& peek(size_t ahead = 0) const {
        return (pos_ + ahead < toks_.size()) ? toks_[pos_ + ahead] : toks_.back();
    }
    const HToken& advance() {
        const HToken& cur = peek();
        if (pos_ + 1 < toks_.size()) ++pos_;
        return cur;
    }
    bool is_type_tok(const HToken& t) const {
        return t.type == HCTok::KW_U0 || t.type == HCTok::KW_I64 ||
               t.type == HCTok::KW_U8 || t.type == HCTok::KW_F64 ||
               (t.type == HCTok::IDENT && known_classes_.count(t.text));
    }
    HToken expect(HCTok t, const char* msg) {
        if (peek().type != t) throw std::runtime_error(std::string("HolyC Parse error (line ") + std::to_string(peek().line) + "): " + msg + ", got '" + peek().text + "'");
        return advance();
    }

    std::shared_ptr<HCExpr> parse_primary() {
        if (peek().type == HCTok::NUMBER) return std::make_shared<HCNumExpr>(std::stoll(advance().text));
        if (peek().type == HCTok::MINUS && peek(1).type == HCTok::NUMBER) {
            advance();
            return std::make_shared<HCNumExpr>(-std::stoll(advance().text));
        }
        if (peek().type == HCTok::IDENT) {
            std::string name = advance().text;
            if (peek().type == HCTok::DOT) {
                advance();
                name += "." + expect(HCTok::IDENT, "field name").text;
                return std::make_shared<HCVarExpr>(name);
            }
            if (peek().type == HCTok::LPAREN) {
                advance();
                auto call = std::make_shared<HCCallExpr>();
                call->callee = name;
                if (peek().type != HCTok::RPAREN) {
                    call->args.push_back(parse_expr());
                    while (peek().type == HCTok::COMMA) { advance(); call->args.push_back(parse_expr()); }
                }
                expect(HCTok::RPAREN, "')'");
                return call;
            }
            if (known_funcs_.count(name) && peek().type == HCTok::SEMI) {
                auto call = std::make_shared<HCCallExpr>();
                call->callee = name;
                return call;
            }
            return std::make_shared<HCVarExpr>(name);
        }
        if (peek().type == HCTok::LPAREN) {
            advance();
            auto e = parse_expr();
            expect(HCTok::RPAREN, "')'");
            return e;
        }
        throw std::runtime_error("HolyC unexpected token in expr at line " + std::to_string(peek().line) + ": '" + peek().text + "'");
    }

    std::shared_ptr<HCExpr> parse_power() {
        auto left = parse_primary();
        while (peek().type == HCTok::BACKTICK_POW) {
            HCTok op = advance().type;
            left = std::make_shared<HCBinExpr>(op, left, parse_primary());
        }
        return left;
    }

    std::shared_ptr<HCExpr> parse_mul() {
        auto left = parse_power();
        while (peek().type == HCTok::MUL || peek().type == HCTok::DIV) {
            HCTok op = advance().type;
            left = std::make_shared<HCBinExpr>(op, left, parse_power());
        }
        return left;
    }

    std::shared_ptr<HCExpr> parse_add() {
        auto left = parse_mul();
        while (peek().type == HCTok::PLUS || peek().type == HCTok::MINUS) {
            HCTok op = advance().type;
            left = std::make_shared<HCBinExpr>(op, left, parse_mul());
        }
        return left;
    }

    static bool is_cmp_tok(HCTok t) {
        return t == HCTok::LE || t == HCTok::LT || t == HCTok::GE ||
               t == HCTok::GT || t == HCTok::EQ || t == HCTok::NE;
    }

    std::shared_ptr<HCExpr> parse_expr() {
        auto first = parse_add();
        if (!is_cmp_tok(peek().type)) return first;
        auto chain = std::make_shared<HCChainedCmpExpr>();
        chain->operands.push_back(first);
        while (is_cmp_tok(peek().type)) {
            chain->ops.push_back(advance().type);
            chain->operands.push_back(parse_add());
        }
        return chain;
    }

    std::shared_ptr<HCStmt> parse_stmt() {
        if (peek().type == HCTok::LBRACE) {
            advance();
            auto blk = std::make_shared<HCBlockStmt>();
            while (peek().type != HCTok::RBRACE && peek().type != HCTok::EOF_TOK) blk->stmts.push_back(parse_stmt());
            expect(HCTok::RBRACE, "'}'");
            return blk;
        }
        if (is_type_tok(peek())) {
            advance();
            std::string name = expect(HCTok::IDENT, "var name").text;
            auto decl = std::make_shared<HCVarDeclStmt>();
            decl->name = name;
            if (peek().type == HCTok::ASSIGN) { advance(); decl->init = parse_expr(); }
            expect(HCTok::SEMI, "';'");
            return decl;
        }
        if (peek().type == HCTok::KW_IF) {
            advance();
            expect(HCTok::LPAREN, "'('");
            auto cond = parse_expr();
            expect(HCTok::RPAREN, "')'");
            auto then_br = parse_stmt();
            std::shared_ptr<HCStmt> else_br;
            if (peek().type == HCTok::KW_ELSE) { advance(); else_br = parse_stmt(); }
            auto s = std::make_shared<HCIfStmt>();
            s->cond = cond; s->then_br = then_br; s->else_br = else_br;
            return s;
        }
        if (peek().type == HCTok::KW_WHILE) {
            advance();
            expect(HCTok::LPAREN, "'('");
            auto cond = parse_expr();
            expect(HCTok::RPAREN, "')'");
            auto body = parse_stmt();
            auto s = std::make_shared<HCWhileStmt>();
            s->cond = cond; s->body = body;
            return s;
        }
        if (peek().type == HCTok::KW_RETURN) {
            advance();
            auto s = std::make_shared<HCReturnStmt>();
            if (peek().type != HCTok::SEMI) s->expr = parse_expr();
            expect(HCTok::SEMI, "';'");
            return s;
        }
        if (peek().type == HCTok::KW_GODTALK) {
            advance();
            expect(HCTok::LPAREN, "'('");
            auto cnt = parse_expr();
            expect(HCTok::RPAREN, "')'");
            expect(HCTok::SEMI, "';'");
            auto s = std::make_shared<HCGodTalkStmt>();
            s->count_expr = cnt;
            return s;
        }
        if (peek().type == HCTok::KW_ASM) {
            advance();
            expect(HCTok::LBRACE, "'{'");
            std::ostringstream asm_txt;
            std::string var_emit = "ch";
            while (peek().type != HCTok::RBRACE && peek().type != HCTok::EOF_TOK) {
                if (peek().text == "MOV" && peek(1).text == "RAX" && peek(2).type == HCTok::COMMA) {
                    var_emit = peek(3).text;
                }
                asm_txt << advance().text << " ";
            }
            expect(HCTok::RBRACE, "'}'");
            auto s = std::make_shared<HCInlineAsmStmt>();
            s->var_to_emit = var_emit;
            s->asm_text = asm_txt.str();
            return s;
        }
        if (peek().type == HCTok::STRING) {
            auto s = std::make_shared<HCPrintStrStmt>();
            s->str = advance().text;
            expect(HCTok::SEMI, "';'");
            return s;
        }
        if (peek().type == HCTok::IDENT) {
            if (peek(1).type == HCTok::ASSIGN || (peek(1).type == HCTok::DOT && peek(3).type == HCTok::ASSIGN)) {
                std::string target = advance().text;
                if (peek().type == HCTok::DOT) { advance(); target += "." + expect(HCTok::IDENT, "field").text; }
                expect(HCTok::ASSIGN, "'='");
                auto val = parse_expr();
                expect(HCTok::SEMI, "';'");
                auto s = std::make_shared<HCAssignStmt>();
                s->name = target; s->val = val;
                return s;
            }
        }
        auto e = parse_expr();
        expect(HCTok::SEMI, "';'");
        auto s = std::make_shared<HCExprStmt>();
        s->expr = e;
        return s;
    }

public:
    explicit HolyCParser(const std::vector<HToken>& toks) : toks_(toks) {}

    HCProgram parse() {
        HCProgram prog;
        while (peek().type != HCTok::EOF_TOK) {
            if (peek().type == HCTok::KW_CLASS) {
                advance();
                std::string cname = expect(HCTok::IDENT, "Class name").text;
                known_classes_.insert(cname);
                prog.class_names.insert(cname);
                expect(HCTok::LBRACE, "'{'");
                while (peek().type != HCTok::RBRACE && peek().type != HCTok::EOF_TOK) advance();
                expect(HCTok::RBRACE, "'}'");
                if (peek().type == HCTok::SEMI) advance();
                continue;
            }
            if (is_type_tok(peek())) {
                std::string rtype = advance().text;
                std::string name = expect(HCTok::IDENT, "decl name").text;
                if (peek().type == HCTok::LPAREN) {
                    known_funcs_.insert(name);
                    HCFuncDecl fn;
                    fn.ret_type = rtype; fn.name = name;
                    advance();
                    while (peek().type != HCTok::RPAREN && peek().type != HCTok::EOF_TOK) {
                        if (is_type_tok(peek())) advance();
                        if (peek().type == HCTok::IDENT) fn.params.push_back(advance().text);
                        if (peek().type == HCTok::COMMA) advance();
                    }
                    expect(HCTok::RPAREN, "')'");
                    fn.body = std::dynamic_pointer_cast<HCBlockStmt>(parse_stmt());
                    prog.functions.push_back(std::move(fn));
                } else {
                    auto g = std::make_shared<HCVarDeclStmt>();
                    g->name = name;
                    if (peek().type == HCTok::ASSIGN) { advance(); g->init = parse_expr(); }
                    expect(HCTok::SEMI, "';'");
                    prog.globals.push_back(std::move(g));
                }
                continue;
            }
            throw std::runtime_error("HolyC top-level unexpected at line " + std::to_string(peek().line) + ": " + peek().text);
        }
        return prog;
    }
};

enum class HCOp : uint8_t {
    PUSH_I64 = 1, LOAD_VAR, STORE_VAR, ADD, SUB, MUL, DIV, POW,
    CMP_CHAIN, JZ, JMP, CALL, RET, PRINT_STR, GOD_TALK, ASM_INT80, POP, HALT
};

struct HCInstr {
    HCOp op;
    int64_t imm = 0;
    std::string sval;
    std::vector<HCTok> cmp_ops;
};

class HolyCCompiler {
    std::vector<HCInstr> code_;
    std::unordered_map<std::string, size_t> fn_addrs_;
    std::vector<std::pair<size_t, std::string>> call_fixups_;

    void compile_expr(const std::shared_ptr<HCExpr>& e) {
        if (auto n = std::dynamic_pointer_cast<HCNumExpr>(e)) code_.push_back({HCOp::PUSH_I64, n->val, {}, {}});
        else if (auto v = std::dynamic_pointer_cast<HCVarExpr>(e)) code_.push_back({HCOp::LOAD_VAR, 0, v->name, {}});
        else if (auto b = std::dynamic_pointer_cast<HCBinExpr>(e)) {
            compile_expr(b->lhs);
            compile_expr(b->rhs);
            HCOp op = (b->op == HCTok::PLUS) ? HCOp::ADD :
                      (b->op == HCTok::MINUS) ? HCOp::SUB :
                      (b->op == HCTok::MUL) ? HCOp::MUL :
                      (b->op == HCTok::DIV) ? HCOp::DIV : HCOp::POW;
            code_.push_back({op, 0, {}, {}});
        } else if (auto cc = std::dynamic_pointer_cast<HCChainedCmpExpr>(e)) {
            for (const auto& opnd : cc->operands) compile_expr(opnd);
            code_.push_back({HCOp::CMP_CHAIN, static_cast<int64_t>(cc->operands.size()), {}, cc->ops});
        } else if (auto c = std::dynamic_pointer_cast<HCCallExpr>(e)) {
            for (const auto& arg : c->args) compile_expr(arg);
            size_t idx = code_.size();
            code_.push_back({HCOp::CALL, static_cast<int64_t>(c->args.size()), c->callee, {}});
            call_fixups_.push_back({idx, c->callee});
        }
    }

    void compile_stmt(const std::shared_ptr<HCStmt>& s) {
        if (auto blk = std::dynamic_pointer_cast<HCBlockStmt>(s)) {
            for (const auto& sub : blk->stmts) compile_stmt(sub);
        } else if (auto vd = std::dynamic_pointer_cast<HCVarDeclStmt>(s)) {
            if (vd->init) { compile_expr(vd->init); code_.push_back({HCOp::STORE_VAR, 0, vd->name, {}}); }
        } else if (auto as = std::dynamic_pointer_cast<HCAssignStmt>(s)) {
            compile_expr(as->val); code_.push_back({HCOp::STORE_VAR, 0, as->name, {}});
        } else if (auto pr = std::dynamic_pointer_cast<HCPrintStrStmt>(s)) {
            code_.push_back({HCOp::PRINT_STR, 0, pr->str, {}});
        } else if (auto gt = std::dynamic_pointer_cast<HCGodTalkStmt>(s)) {
            compile_expr(gt->count_expr); code_.push_back({HCOp::GOD_TALK, 0, {}, {}});
        } else if (auto ia = std::dynamic_pointer_cast<HCInlineAsmStmt>(s)) {
            if (!ia->var_to_emit.empty() && std::isdigit(static_cast<unsigned char>(ia->var_to_emit[0]))) {
                code_.push_back({HCOp::PUSH_I64, std::stoll(ia->var_to_emit), {}, {}});
            } else {
                code_.push_back({HCOp::LOAD_VAR, 0, ia->var_to_emit, {}});
            }
            code_.push_back({HCOp::ASM_INT80, 0, ia->asm_text, {}});
        } else if (auto ret = std::dynamic_pointer_cast<HCReturnStmt>(s)) {
            if (ret->expr) compile_expr(ret->expr);
            else code_.push_back({HCOp::PUSH_I64, 0, {}, {}});
            code_.push_back({HCOp::RET, 0, {}, {}});
        } else if (auto ifs = std::dynamic_pointer_cast<HCIfStmt>(s)) {
            compile_expr(ifs->cond);
            size_t jz_idx = code_.size();
            code_.push_back({HCOp::JZ, 0, {}, {}});
            compile_stmt(ifs->then_br);
            if (ifs->else_br) {
                size_t jmp_idx = code_.size();
                code_.push_back({HCOp::JMP, 0, {}, {}});
                code_[jz_idx].imm = static_cast<int64_t>(code_.size());
                compile_stmt(ifs->else_br);
                code_[jmp_idx].imm = static_cast<int64_t>(code_.size());
            } else {
                code_[jz_idx].imm = static_cast<int64_t>(code_.size());
            }
        } else if (auto wh = std::dynamic_pointer_cast<HCWhileStmt>(s)) {
            size_t loop_top = code_.size();
            compile_expr(wh->cond);
            size_t jz_idx = code_.size();
            code_.push_back({HCOp::JZ, 0, {}, {}});
            compile_stmt(wh->body);
            code_.push_back({HCOp::JMP, static_cast<int64_t>(loop_top), {}, {}});
            code_[jz_idx].imm = static_cast<int64_t>(code_.size());
        } else if (auto es = std::dynamic_pointer_cast<HCExprStmt>(s)) {
            compile_expr(es->expr); code_.push_back({HCOp::POP, 0, {}, {}});
        }
    }

public:
    std::vector<HCInstr> compile(const HCProgram& prog) {
        for (const auto& g : prog.globals) {
            if (g->init) { compile_expr(g->init); code_.push_back({HCOp::STORE_VAR, 0, g->name, {}}); }
        }
        size_t main_call = code_.size();
        code_.push_back({HCOp::CALL, 0, "Main", {}});
        call_fixups_.push_back({main_call, "Main"});
        code_.push_back({HCOp::HALT, 0, {}, {}});

        for (const auto& fn : prog.functions) {
            fn_addrs_[fn.name] = code_.size();
            for (auto it = fn.params.rbegin(); it != fn.params.rend(); ++it) {
                code_.push_back({HCOp::STORE_VAR, 0, *it, {}});
            }
            compile_stmt(fn.body);
            code_.push_back({HCOp::PUSH_I64, 0, {}, {}});
            code_.push_back({HCOp::RET, 0, {}, {}});
        }

        for (const auto& [idx, name] : call_fixups_) {
            if (!fn_addrs_.count(name)) throw std::runtime_error("Undefined HolyC function: " + name);
            code_[idx].imm = static_cast<int64_t>(fn_addrs_.at(name));
        }
        return code_;
    }
};

std::string strip_doldoc_markup(const std::string& raw, std::ostringstream& doldoc_log) {
    std::string out;
    for (size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '$') {
            size_t end = raw.find('$', i + 1);
            if (end != std::string::npos) {
                std::string tag = raw.substr(i + 1, end - i - 1);
                if (!tag.empty()) doldoc_log << "[DolDoc $" << tag << "$]\r\n";
                i = end;
                continue;
            }
        }
        out.push_back(raw[i]);
    }
    return out;
}

const char* hc_op_name(HCOp op) {
    switch (op) {
        case HCOp::PUSH_I64: return "HC_PUSH_I64";
        case HCOp::LOAD_VAR: return "HC_LOAD_VAR";
        case HCOp::STORE_VAR: return "HC_STORE_VAR";
        case HCOp::ADD: return "HC_ADD";
        case HCOp::SUB: return "HC_SUB";
        case HCOp::MUL: return "HC_MUL";
        case HCOp::DIV: return "HC_DIV";
        case HCOp::POW: return "HC_POW (`)";
        case HCOp::CMP_CHAIN: return "HC_CMP_CHAIN";
        case HCOp::JZ: return "HC_JZ";
        case HCOp::JMP: return "HC_JMP";
        case HCOp::CALL: return "HC_CALL";
        case HCOp::RET: return "HC_RET";
        case HCOp::PRINT_STR: return "HC_PRINT_STR";
        case HCOp::GOD_TALK: return "HC_GOD_TALK";
        case HCOp::ASM_INT80: return "HC_ASM_INT80";
        case HCOp::POP: return "HC_POP";
        case HCOp::HALT: return "HC_HALT";
        default: return "HC_UNK";
    }
}

HolyCCompileRunResult run_holyc_pipeline_core(
    const std::string& raw_src,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
) {
    HolyCCompileRunResult res;
    try {
        std::string exe_log;
        std::string expanded_src = run_holyc_compile_time_exe_blocks(raw_src, exe_log);
        cb(25, "[HOLYC-#exe] Выполнен compile-time блок #exe { StreamPrint(...) }");
        sleep_cb(100);

        auto toks = lex_holyc(expanded_src);
        HolyCParser parser(toks);
        auto prog = parser.parse();
        HolyCCompiler compiler;
        auto bytecode = compiler.compile(prog);
        cb(60, "[HOLYC-CC] Скомпилирован HolyC в " + std::to_string(bytecode.size()) + " опкодов");
        sleep_cb(100);

        struct Frame { size_t ret_ip; std::unordered_map<std::string, int64_t> locals; };
        std::unordered_map<std::string, int64_t> globals;
        std::vector<Frame> call_stack;
        std::vector<int64_t> stack;
        std::ostringstream vm_log, printed_ss;

        vm_log << exe_log << "\r\n=== HOLYC VM BYTECODE (" << bytecode.size() << " ops) ===\r\n";
        for (size_t i = 0; i < bytecode.size() && i < 45; ++i) {
            vm_log << std::setw(3) << i << ": " << hc_op_name(bytecode[i].op);
            if (!bytecode[i].sval.empty()) vm_log << " \"" << bytecode[i].sval << "\"";
            else if (bytecode[i].imm != 0) vm_log << " " << bytecode[i].imm;
            vm_log << "\r\n";
        }
        vm_log << "\r\n=== HOLYC VM OUTPUT & DOLDOC LOG ===\r\n";

        auto get_var = [&](const std::string& name) -> int64_t {
            if (!call_stack.empty()) {
                auto it = call_stack.back().locals.find(name);
                if (it != call_stack.back().locals.end()) return it->second;
            }
            return globals[name];
        };
        auto set_var = [&](const std::string& name, int64_t v) {
            if (!call_stack.empty() && globals.find(name) == globals.end()) call_stack.back().locals[name] = v;
            else globals[name] = v;
        };

        size_t ip = 0, cycles = 0;
        bool halted = false;
        while (!halted && !cancel_flag.load() && ip < bytecode.size() && cycles < 50000) {
            const auto& ins = bytecode[ip++];
            ++cycles;
            switch (ins.op) {
                case HCOp::PUSH_I64: stack.push_back(ins.imm); break;
                case HCOp::LOAD_VAR: stack.push_back(get_var(ins.sval)); break;
                case HCOp::STORE_VAR: { int64_t v = stack.back(); stack.pop_back(); set_var(ins.sval, v); break; }
                case HCOp::ADD: { int64_t b = stack.back(); stack.pop_back(); stack.back() += b; break; }
                case HCOp::SUB: { int64_t b = stack.back(); stack.pop_back(); stack.back() -= b; break; }
                case HCOp::MUL: { int64_t b = stack.back(); stack.pop_back(); stack.back() *= b; break; }
                case HCOp::DIV: { int64_t b = stack.back(); stack.pop_back(); stack.back() = (b != 0) ? (stack.back() / b) : 0; break; }
                case HCOp::POW: {
                    int64_t e = stack.back(); stack.pop_back();
                    int64_t base = stack.back(); stack.pop_back();
                    int64_t r = 1;
                    for (int64_t k = 0; k < e; ++k) r *= base;
                    stack.push_back(r);
                    break;
                }
                case HCOp::CMP_CHAIN: {
                    size_t cnt = static_cast<size_t>(ins.imm);
                    std::vector<int64_t> vals(cnt);
                    for (size_t k = cnt; k > 0; --k) { vals[k - 1] = stack.back(); stack.pop_back(); }
                    bool ok = true;
                    for (size_t k = 0; k < ins.cmp_ops.size(); ++k) {
                        int64_t a = vals[k], b = vals[k + 1];
                        if (ins.cmp_ops[k] == HCTok::LE) ok = ok && (a <= b);
                        else if (ins.cmp_ops[k] == HCTok::LT) ok = ok && (a < b);
                        else if (ins.cmp_ops[k] == HCTok::GE) ok = ok && (a >= b);
                        else if (ins.cmp_ops[k] == HCTok::GT) ok = ok && (a > b);
                        else if (ins.cmp_ops[k] == HCTok::EQ) ok = ok && (a == b);
                        else if (ins.cmp_ops[k] == HCTok::NE) ok = ok && (a != b);
                    }
                    stack.push_back(ok ? 1 : 0);
                    break;
                }
                case HCOp::JZ: { int64_t v = stack.back(); stack.pop_back(); if (v == 0) ip = static_cast<size_t>(ins.imm); break; }
                case HCOp::JMP: ip = static_cast<size_t>(ins.imm); break;
                case HCOp::CALL: call_stack.push_back({ip, {}}); ip = static_cast<size_t>(ins.imm); break;
                case HCOp::RET: ip = call_stack.back().ret_ip; call_stack.pop_back(); break;
                case HCOp::PRINT_STR: {
                    std::string clean = strip_doldoc_markup(ins.sval, vm_log);
                    vm_log << clean << "\r\n";
                    printed_ss << clean;
                    break;
                }
                case HCOp::GOD_TALK: {
                    int64_t cnt = stack.back(); stack.pop_back();
                    static const char* kGodWords[] = {
                        "TEMPLE", "RING0", "ASSEMBLY", "CHARIOT", "LIGHTNING",
                        "DIVINE", "COMPILER", "ELEPHANT", "PSALM", "RAINBOW",
                        "TRUMPET", "ORACLE", "BLESSED", "VGA16", "KERNEL"
                    };
                    uint64_t rng = __rdtsc() ^ 0x9E3779B97F4A7C15ULL;
                    std::ostringstream gw;
                    for (int64_t k = 0; k < cnt; ++k) {
                        rng = rng * 6364136223846793005ULL + 1442695040888963407ULL;
                        if (k > 0) gw << ' ';
                        gw << kGodWords[(rng >> 32) % 15];
                    }
                    res.god_oracle_words = gw.str();
                    vm_log << "[GodTalk()]: " << res.god_oracle_words << "\r\n";
                    cb(80, "[HOLYC-GODWORDS] Terry RNG Oracle: \"" + res.god_oracle_words + "\"");
                    break;
                }
                case HCOp::ASM_INT80: {
                    int64_t ch = stack.back(); stack.pop_back();
                    res.emitted_output.push_back(static_cast<char>(ch & 0xFF));
                    vm_log << "[asm { MOV RAX," << ch << "; INT 0x80 }] -> '" << static_cast<char>(ch) << "'\r\n";
                    break;
                }
                case HCOp::POP: if (!stack.empty()) stack.pop_back(); break;
                case HCOp::HALT: halted = true; break;
            }
        }

        res.printed_text = printed_ss.str();
        res.raw_and_expanded_source =
            "=== RAW HOLYC SOURCE (.HC) ===\r\n" + raw_src +
            "\r\n=== AFTER #exe { StreamPrint(...) } ===\r\n" + expanded_src;
        res.vm_and_doldoc_report = vm_log.str();
        res.opcodes_count = bytecode.size();
        res.vm_cycles = cycles;
        cb(100, "[HOLYC-VM] HolyC VM выполнил " + std::to_string(cycles) + " тактов -> \"" + res.emitted_output + "\"");
        sleep_cb(100);
        return res;
    } catch (const std::exception& ex) {
        res.success = false;
        res.error_msg = ex.what();
        return res;
    }
}

} // namespace

HolyCCompileRunResult compile_and_run_holyc_stage(
    const std::string& payload,
    std::atomic<bool>& cancel_flag,
    const std::function<void(int pct, const std::string& log_msg)>& cb,
    const std::function<void(int ms)>& sleep_cb
) {
    std::ostringstream hc_src;
    hc_src << "$FG,LTGREEN$// === AUTOGENERATED RUNTIME HOLYC MODULE ===$FG$\n"
           << "Class CHolyConfig {\n  I64 magic_power;\n  I64 ring_level;\n  U8  blessed;\n};\n\n"
           << "#exe {\n"
           << "  StreamPrint(\"I64 HOLY_SHIFT = 2`3;\\n\");\n"
           << "  StreamPrint(\"U0 EmitDivinePayload() {\\n\");\n";
    for (size_t i = 0; i < payload.size(); ++i) {
        int val = static_cast<unsigned char>(payload[i]);
        hc_src << "  StreamPrint(\"  TempleEmitChar(" << (val - 8) << " + HOLY_SHIFT);\\n\");\n";
    }
    hc_src << "  StreamPrint(\"}\\n\");\n}\n\n"
           << "U0 BlessRingZero() {\n  \"$FG,YELLOW$[HolyC VM] Ring-0 Active!$FG$\\n\";\n}\n\n"
           << "I64 ValidatePrintableAscii(I64 ch) {\n  if (32 <= ch <= 126) {\n    return 1;\n  }\n  return 0;\n}\n\n"
           << "U0 TempleEmitChar(U8 ch) {\n  if (ValidatePrintableAscii(ch)) {\n    asm {\n      MOV RAX, ch\n      INT 0x80\n    }\n  }\n}\n\n"
           << "U0 Main() {\n  CHolyConfig cfg;\n  cfg.magic_power = HOLY_SHIFT;\n  BlessRingZero;\n  GodTalk(5);\n  EmitDivinePayload;\n}\n";
    return run_holyc_pipeline_core(hc_src.str(), cancel_flag, cb, sleep_cb);
}

HolyCCompileRunResult execute_custom_holyc_source(const std::string& user_holyc_src) {
    std::atomic<bool> cancel{false};
    return run_holyc_pipeline_core(
        user_holyc_src,
        cancel,
        [](int, const std::string&) {},
        [](int) {}
    );
}

} // namespace govno::holyc
