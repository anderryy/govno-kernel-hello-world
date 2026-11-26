#include "compiler/govno_compiler.hpp"
#include "madness/subleq_oisc.hpp"
#include "cursed_macros.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <unordered_map>

namespace govno::compiler {

constexpr auto kCompileTimeWatermark =
    govno::metaprogramming::EncryptedCompileTimeBlob("GOVNO-COMPILER-CORE-v3.0-APOCALYPSE", 27);

uint32_t compute_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            uint32_t mask = (crc & 1u) ? 0xEDB88320u : 0u;
            crc = (crc >> 1u) ^ mask;
        }
    }
    return ~crc;
}

namespace {

std::unordered_map<std::string, std::string> parse_simple_xml_tags(const std::string& xml) {
    std::unordered_map<std::string, std::string> out;
    size_t pos = 0;
    while (pos < xml.size()) {
        size_t open = xml.find('<', pos);
        if (open == std::string::npos) break;
        size_t close = xml.find('>', open + 1);
        if (close == std::string::npos) break;
        std::string tag = xml.substr(open + 1, close - open - 1);
        if (!tag.empty() && tag[0] != '/' && tag[0] != '?') {
            size_t sp = tag.find(' ');
            std::string tag_name = (sp == std::string::npos) ? tag : tag.substr(0, sp);
            std::string end_tok = "</" + tag_name + ">";
            size_t end_pos = xml.find(end_tok, close + 1);
            if (end_pos != std::string::npos) {
                std::string inner = xml.substr(close + 1, end_pos - (close + 1));
                if (inner.find('<') == std::string::npos) out[tag_name] = inner;
            }
        }
        pos = close + 1;
    }
    return out;
}

struct SqlRow {
    int id;
    std::string key_name;
    int value;
    int active;
};

std::vector<SqlRow> execute_govnosql_select(
    const std::vector<SqlRow>& table,
    const std::string& sql_query,
    size_t& out_scanned
) {
    out_scanned = table.size();
    bool filter_active = (sql_query.find("WHERE active = 1") != std::string::npos);
    std::vector<SqlRow> result;
    for (const auto& r : table) {
        if (!filter_active || r.active == 1) result.push_back(r);
    }
    return result;
}

std::string preprocess_govnoscript(const std::string& raw_src, size_t& out_macro_count) {
    std::unordered_map<std::string, std::string> defines;
    std::istringstream iss(raw_src);
    std::ostringstream out;
    std::string line;
    out_macro_count = 0;

    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("#DEFINE ", 0) == 0) {
            std::istringstream ls(line.substr(8));
            std::string name, val;
            ls >> name >> val;
            if (!name.empty() && !val.empty()) {
                defines[name] = val;
                ++out_macro_count;
            }
            continue;
        }
        for (const auto& [k, v] : defines) {
            size_t p = 0;
            while ((p = line.find(k, p)) != std::string::npos) {
                bool left_ok = (p == 0 || (!std::isalnum(static_cast<unsigned char>(line[p - 1])) && line[p - 1] != '_'));
                size_t end_p = p + k.size();
                bool right_ok = (end_p >= line.size() || (!std::isalnum(static_cast<unsigned char>(line[end_p])) && line[end_p] != '_'));
                if (left_ok && right_ok) {
                    line.replace(p, k.size(), v);
                    p += v.size();
                } else {
                    p += k.size();
                }
            }
        }
        out << line << "\r\n";
    }
    return out.str();
}

enum class TokenType {
    TOK_EOF = 0,
    TOK_KW_KERNEL_MODULE,
    TOK_KW_RESERVE_PAGES,
    TOK_KW_VAR,
    TOK_KW_SYSCALL_EMIT,
    TOK_KW_HALT,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_ASSIGN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_SEMICOLON
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line = 1;
    int col = 1;
};

const char* token_type_name(TokenType t) {
    switch (t) {
        case TokenType::TOK_EOF: return "EOF";
        case TokenType::TOK_KW_KERNEL_MODULE: return "KW_KERNEL_MODULE";
        case TokenType::TOK_KW_RESERVE_PAGES: return "KW_RESERVE_PAGES";
        case TokenType::TOK_KW_VAR: return "KW_VAR";
        case TokenType::TOK_KW_SYSCALL_EMIT: return "KW_SYSCALL_EMIT";
        case TokenType::TOK_KW_HALT: return "KW_HALT";
        case TokenType::TOK_IDENT: return "IDENT";
        case TokenType::TOK_NUMBER: return "NUMBER";
        case TokenType::TOK_STRING: return "STRING";
        case TokenType::TOK_ASSIGN: return "ASSIGN";
        case TokenType::TOK_PLUS: return "PLUS";
        case TokenType::TOK_MINUS: return "MINUS";
        case TokenType::TOK_MUL: return "MUL";
        case TokenType::TOK_DIV: return "DIV";
        case TokenType::TOK_LPAREN: return "LPAREN";
        case TokenType::TOK_RPAREN: return "RPAREN";
        case TokenType::TOK_SEMICOLON: return "SEMICOLON";
        default: return "UNKNOWN";
    }
}

std::vector<Token> lex_govnoscript(const std::string& src) {
    std::vector<Token> tokens;
    size_t i = 0;
    int line = 1, col = 1;
    const size_t n = src.size();

    METKA(lex_dispatch)
    ESLI_POVEZET i >= n TO_TOGDA
        tokens.push_back({TokenType::TOK_EOF, "<EOF>", line, col});
        VOZVRAT_STRADANIYA tokens;
    KONEC_ESLI

    char c = src[i];
    if (c == ' ' || c == '\t' || c == '\r') { ++i; ++col; PRIGNUT_NA(lex_dispatch); }
    if (c == '\n') { ++i; ++line; col = 1; PRIGNUT_NA(lex_dispatch); }
    if (c == '/' && i + 1 < n && src[i + 1] == '/') {
        while (i < n && src[i] != '\n') ++i;
        PRIGNUT_NA(lex_dispatch);
    }
    if (c == '"') {
        int start_col = col;
        ++i; ++col;
        std::string val;
        while (i < n && src[i] != '"') { val.push_back(src[i]); ++i; ++col; }
        if (i < n && src[i] == '"') { ++i; ++col; }
        tokens.push_back({TokenType::TOK_STRING, val, line, start_col});
        PRIGNUT_NA(lex_dispatch);
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
        int start_col = col;
        std::string num;
        while (i < n && std::isdigit(static_cast<unsigned char>(src[i]))) {
            num.push_back(src[i]); ++i; ++col;
        }
        tokens.push_back({TokenType::TOK_NUMBER, num, line, start_col});
        PRIGNUT_NA(lex_dispatch);
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        int start_col = col;
        std::string id;
        while (i < n && (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_')) {
            id.push_back(src[i]); ++i; ++col;
        }
        TokenType kw = TokenType::TOK_IDENT;
        if (id == "KERNEL_MODULE") kw = TokenType::TOK_KW_KERNEL_MODULE;
        else if (id == "RESERVE_PAGE_FRAMES") kw = TokenType::TOK_KW_RESERVE_PAGES;
        else if (id == "VAR") kw = TokenType::TOK_KW_VAR;
        else if (id == "SYSCALL_EMIT") kw = TokenType::TOK_KW_SYSCALL_EMIT;
        else if (id == "HALT_KERNEL_UNIVERSE") kw = TokenType::TOK_KW_HALT;
        tokens.push_back({kw, id, line, start_col});
        PRIGNUT_NA(lex_dispatch);
    }

    TokenType single = TokenType::TOK_EOF;
    switch (c) {
        case '=': single = TokenType::TOK_ASSIGN; break;
        case '+': single = TokenType::TOK_PLUS; break;
        case '-': single = TokenType::TOK_MINUS; break;
        case '*': single = TokenType::TOK_MUL; break;
        case '/': single = TokenType::TOK_DIV; break;
        case '(': single = TokenType::TOK_LPAREN; break;
        case ')': single = TokenType::TOK_RPAREN; break;
        case ';': single = TokenType::TOK_SEMICOLON; break;
        default: throw std::runtime_error(std::string("Lexer error on: ") + c);
    }
    tokens.push_back({single, std::string(1, c), line, col});
    ++i; ++col;
    PRIGNUT_NA(lex_dispatch);
}

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void dump(std::ostringstream& os, int indent, size_t& node_counter) const = 0;
};
struct ExprNode : ASTNode {};
struct StmtNode : ASTNode {};

struct NumberExpr : ExprNode {
    int value;
    explicit NumberExpr(int v) : value(v) {}
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "NumberLiteral(" << value << ")\r\n";
    }
};

struct IdentExpr : ExprNode {
    std::string name;
    explicit IdentExpr(std::string n) : name(std::move(n)) {}
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "IdentRef(" << name << ")\r\n";
    }
};

struct BinaryExpr : ExprNode {
    char op;
    std::shared_ptr<ExprNode> lhs, rhs;
    BinaryExpr(char o, std::shared_ptr<ExprNode> l, std::shared_ptr<ExprNode> r)
        : op(o), lhs(std::move(l)), rhs(std::move(r)) {}
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "BinaryOp('" << op << "')\r\n";
        lhs->dump(os, indent + 1, node_counter);
        rhs->dump(os, indent + 1, node_counter);
    }
};

struct ModuleStmt : StmtNode {
    std::string module_name;
    explicit ModuleStmt(std::string m) : module_name(std::move(m)) {}
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "KernelModuleDecl(\"" << module_name << "\")\r\n";
    }
};

struct ReservePagesStmt : StmtNode {
    int pages;
    explicit ReservePagesStmt(int p) : pages(p) {}
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "ReservePageFrames(" << pages << ")\r\n";
    }
};

struct VarDeclStmt : StmtNode {
    std::string name;
    std::shared_ptr<ExprNode> init_expr;
    VarDeclStmt(std::string n, std::shared_ptr<ExprNode> e) : name(std::move(n)), init_expr(std::move(e)) {}
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "VarDecl(" << name << ")\r\n";
        init_expr->dump(os, indent + 1, node_counter);
    }
};

struct EmitStmt : StmtNode {
    std::shared_ptr<ExprNode> expr;
    explicit EmitStmt(std::shared_ptr<ExprNode> e) : expr(std::move(e)) {}
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "SyscallEmitChar\r\n";
        expr->dump(os, indent + 1, node_counter);
    }
};

struct HaltStmt : StmtNode {
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "HaltKernelUniverse()\r\n";
    }
};

struct ProgramAST : ASTNode {
    std::vector<std::shared_ptr<StmtNode>> statements;
    void dump(std::ostringstream& os, int indent, size_t& node_counter) const override {
        ++node_counter;
        os << std::string(indent * 2, ' ') << "ProgramRoot\r\n";
        for (const auto& s : statements) s->dump(os, indent + 1, node_counter);
    }
};

class GovnoParser {
    const std::vector<Token>& tokens_;
    size_t pos_ = 0;

    const Token& peek() const { return pos_ < tokens_.size() ? tokens_[pos_] : tokens_.back(); }
    const Token& advance() {
        const Token& cur = peek();
        if (pos_ + 1 < tokens_.size()) ++pos_;
        return cur;
    }
    Token expect(TokenType t, const char* msg) {
        if (peek().type != t) throw std::runtime_error(std::string("Parse error: ") + msg);
        return advance();
    }

    std::shared_ptr<ExprNode> parse_factor() {
        if (peek().type == TokenType::TOK_NUMBER) return std::make_shared<NumberExpr>(std::stoi(advance().lexeme));
        if (peek().type == TokenType::TOK_IDENT) return std::make_shared<IdentExpr>(advance().lexeme);
        if (peek().type == TokenType::TOK_LPAREN) {
            advance();
            auto inner = parse_expr();
            expect(TokenType::TOK_RPAREN, "expected ')'");
            return inner;
        }
        throw std::runtime_error("Unexpected token in factor: " + peek().lexeme);
    }
    std::shared_ptr<ExprNode> parse_term() {
        auto left = parse_factor();
        while (peek().type == TokenType::TOK_MUL || peek().type == TokenType::TOK_DIV) {
            char op = advance().lexeme[0];
            left = std::make_shared<BinaryExpr>(op, left, parse_factor());
        }
        return left;
    }
    std::shared_ptr<ExprNode> parse_expr() {
        auto left = parse_term();
        while (peek().type == TokenType::TOK_PLUS || peek().type == TokenType::TOK_MINUS) {
            char op = advance().lexeme[0];
            left = std::make_shared<BinaryExpr>(op, left, parse_term());
        }
        return left;
    }
    std::shared_ptr<StmtNode> parse_statement() {
        if (peek().type == TokenType::TOK_KW_KERNEL_MODULE) {
            advance();
            auto name = expect(TokenType::TOK_STRING, "expected module string");
            expect(TokenType::TOK_SEMICOLON, "expected ';'");
            return std::make_shared<ModuleStmt>(name.lexeme);
        }
        if (peek().type == TokenType::TOK_KW_RESERVE_PAGES) {
            advance();
            auto num = expect(TokenType::TOK_NUMBER, "expected page count");
            expect(TokenType::TOK_SEMICOLON, "expected ';'");
            return std::make_shared<ReservePagesStmt>(std::stoi(num.lexeme));
        }
        if (peek().type == TokenType::TOK_KW_VAR) {
            advance();
            auto id = expect(TokenType::TOK_IDENT, "expected variable identifier");
            expect(TokenType::TOK_ASSIGN, "expected '='");
            auto expr = parse_expr();
            expect(TokenType::TOK_SEMICOLON, "expected ';'");
            return std::make_shared<VarDeclStmt>(id.lexeme, expr);
        }
        if (peek().type == TokenType::TOK_KW_SYSCALL_EMIT) {
            advance();
            auto expr = parse_expr();
            expect(TokenType::TOK_SEMICOLON, "expected ';'");
            return std::make_shared<EmitStmt>(expr);
        }
        if (peek().type == TokenType::TOK_KW_HALT) {
            advance();
            expect(TokenType::TOK_SEMICOLON, "expected ';'");
            return std::make_shared<HaltStmt>();
        }
        throw std::runtime_error("Unknown statement: " + peek().lexeme);
    }

public:
    explicit GovnoParser(const std::vector<Token>& tokens) : tokens_(tokens) {}
    std::shared_ptr<ProgramAST> parse_program() {
        auto prog = std::make_shared<ProgramAST>();
        while (peek().type != TokenType::TOK_EOF) prog->statements.push_back(parse_statement());
        return prog;
    }
};

struct IRInstruction {
    std::string op, dst, src1, src2;
    int imm = 0;
};

std::shared_ptr<ExprNode> deoptimize_expr(const std::shared_ptr<ExprNode>& node) {
    if (auto num = std::dynamic_pointer_cast<NumberExpr>(node)) {
        if (num->value > 5) {
            auto add13 = std::make_shared<BinaryExpr>('+', std::make_shared<NumberExpr>(num->value), std::make_shared<NumberExpr>(13));
            return std::make_shared<BinaryExpr>('-', add13, std::make_shared<NumberExpr>(13));
        }
        return node;
    }
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(node)) {
        return std::make_shared<BinaryExpr>(bin->op, deoptimize_expr(bin->lhs), deoptimize_expr(bin->rhs));
    }
    return node;
}

class IRBuilder {
    int temp_counter_ = 0;
    std::vector<IRInstruction> ir_;
    std::unordered_map<std::string, int> sym_env_;
    std::string new_temp() { return "%t" + std::to_string(temp_counter_++); }

    std::pair<std::string, int> lower_expr(const std::shared_ptr<ExprNode>& expr) {
        if (auto num = std::dynamic_pointer_cast<NumberExpr>(expr)) {
            std::string t = new_temp();
            ir_.push_back({"LOAD_IMM", t, "", "", num->value});
            return {t, num->value};
        }
        if (auto id = std::dynamic_pointer_cast<IdentExpr>(expr)) {
            std::string t = new_temp();
            ir_.push_back({"LOAD_VAR", t, id->name, "", sym_env_.at(id->name)});
            return {t, sym_env_.at(id->name)};
        }
        if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
            auto [l_reg, l_val] = lower_expr(bin->lhs);
            auto [r_reg, r_val] = lower_expr(bin->rhs);
            std::string dst = new_temp();
            int res = 0;
            std::string op_name = "ADD";
            if (bin->op == '+') { res = govno::metaprogramming::enterprise_cursed_add(l_val, r_val); op_name = "CURSED_ADD"; }
            else if (bin->op == '-') { res = l_val - r_val; op_name = "SUB"; }
            else if (bin->op == '*') { res = l_val * r_val; op_name = "MUL"; }
            else if (bin->op == '/') { res = (r_val != 0) ? (l_val / r_val) : 0; op_name = "DIV"; }
            ir_.push_back({op_name, dst, l_reg, r_reg, res});
            return {dst, res};
        }
        throw std::runtime_error("Unknown ExprNode in IRBuilder");
    }

public:
    std::vector<uint8_t> compile_ast_to_ir(const std::shared_ptr<ProgramAST>& ast, std::vector<IRInstruction>& out_ir) {
        std::vector<uint8_t> emitted_bytes;
        ir_.push_back({"DEAD_COMPUTE_PI", "%pi_junk", "3", "1", 314159});
        for (const auto& stmt : ast->statements) {
            if (auto mod = std::dynamic_pointer_cast<ModuleStmt>(stmt)) {
                ir_.push_back({"META_MODULE", mod->module_name, "", "", 0});
            } else if (auto res = std::dynamic_pointer_cast<ReservePagesStmt>(stmt)) {
                ir_.push_back({"KERNEL_ALLOC_PAGES", "%kp", "", "", res->pages});
            } else if (auto var = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
                auto [reg, val] = lower_expr(deoptimize_expr(var->init_expr));
                sym_env_[var->name] = val;
                ir_.push_back({"STORE_VAR", var->name, reg, "", val});
                ir_.push_back({"NOP_SUFFERING", "%void", "", "", 0});
            } else if (auto em = std::dynamic_pointer_cast<EmitStmt>(stmt)) {
                auto [reg, val] = lower_expr(deoptimize_expr(em->expr));
                uint8_t ch = static_cast<uint8_t>(val & 0xFF);
                emitted_bytes.push_back(ch);
                ir_.push_back({"SYS_EMIT_CHAR", reg, "", "", static_cast<int>(ch)});
            } else if (std::dynamic_pointer_cast<HaltStmt>(stmt)) {
                ir_.push_back({"SYS_HALT", "", "", "", 0});
            }
        }
        out_ir = ir_;
        return emitted_bytes;
    }
};

std::pair<std::string, std::vector<uint8_t>> compile_bytes_to_lisp_and_c(
    const std::vector<uint8_t>& bytes,
    size_t& out_lisp_forms,
    size_t& out_c_lines
) {
    std::ostringstream lisp;
    lisp << "; === GOVNO-LISP S-EXPRESSIONS ===\r\n(defun emit-enterprise-hello ()\r\n  (list\r\n";
    std::vector<uint8_t> out_bytes;
    out_lisp_forms = 2;

    for (size_t i = 0; i < bytes.size(); ++i) {
        int val = static_cast<int>(bytes[i]);
        int half = val / 2;
        int rest = val - half;
        lisp << "    (emit-byte (+ " << half << " " << rest << ")) ; '" << static_cast<char>(bytes[i]) << "'\r\n";
        out_bytes.push_back(static_cast<uint8_t>(half + rest));
        ++out_lisp_forms;
    }
    lisp << "  ))\r\n\r\n";

    std::ostringstream csrc;
    csrc << "/* === AUTO-GENERATED C99 SOURCE === */\r\n#include <stdio.h>\r\nint generated_hello(char* buf) {\r\n";
    out_c_lines = 3;
    for (size_t i = 0; i < out_bytes.size(); ++i) {
        int v = static_cast<int>(out_bytes[i]);
        csrc << "    buf[" << i << "] = (char)((" << v << " ^ 0x5A) ^ 0x5A);\r\n";
        ++out_c_lines;
    }
    csrc << "    buf[" << out_bytes.size() << "] = '\\0';\r\n    return " << out_bytes.size() << ";\r\n}\r\n";
    out_c_lines += 3;
    return {lisp.str() + csrc.str(), out_bytes};
}

std::string generate_brainfuck_from_bytes(const std::vector<uint8_t>& bytes) {
    std::string bf;
    int current = 0;
    for (uint8_t target_u8 : bytes) {
        int target = static_cast<int>(target_u8);
        int diff = target - current;
        if (std::abs(diff) < 12) {
            bf.push_back('>');
            char op = (diff >= 0) ? '+' : '-';
            bf.append(static_cast<size_t>(std::abs(diff)), op);
            bf.push_back('.');
            bf.push_back('<');
        } else {
            int mag = std::abs(diff);
            int a = static_cast<int>(std::sqrt(static_cast<double>(mag)));
            if (a < 2) a = 2;
            int b = mag / a;
            int rem = mag % a;
            char op = (diff >= 0) ? '+' : '-';
            bf.append(static_cast<size_t>(a), '+');
            bf.append("[>");
            bf.append(static_cast<size_t>(b), op);
            bf.append("<-]>");
            bf.append(static_cast<size_t>(rem), op);
            bf.push_back('.');
            bf.push_back('<');
        }
        current = target;
    }
    return bf;
}

std::string optimize_brainfuck_peephole(const std::string& raw_bf) {
    std::string opt;
    for (char c : raw_bf) {
        if (!opt.empty()) {
            char p = opt.back();
            if ((p == '+' && c == '-') || (p == '-' && c == '+') ||
                (p == '>' && c == '<') || (p == '<' && c == '>')) {
                opt.pop_back();
                continue;
            }
        }
        opt.push_back(c);
    }
    return opt;
}

std::string verify_brainfuck_execution(const std::string& bf) {
    std::array<uint8_t, 256> tape{};
    size_t ptr = 0;
    std::unordered_map<size_t, size_t> jump_table;
    std::stack<size_t> stk;
    for (size_t i = 0; i < bf.size(); ++i) {
        if (bf[i] == '[') stk.push(i);
        else if (bf[i] == ']') { size_t open = stk.top(); stk.pop(); jump_table[open] = i; jump_table[i] = open; }
    }
    std::string out;
    size_t guard = 0;
    for (size_t ip = 0; ip < bf.size() && guard < 100000; ++ip, ++guard) {
        switch (bf[ip]) {
            case '+': ++tape[ptr]; break;
            case '-': --tape[ptr]; break;
            case '>': ptr = (ptr + 1) % tape.size(); break;
            case '<': ptr = (ptr == 0) ? (tape.size() - 1) : (ptr - 1); break;
            case '[': if (tape[ptr] == 0) ip = jump_table[ip]; break;
            case ']': if (tape[ptr] != 0) ip = jump_table[ip]; break;
            case '.': out.push_back(static_cast<char>(tape[ptr])); break;
            default: break;
        }
    }
    return out;
}

struct AsmLine {
    std::string label, mnemonic, operand_str;
    uint32_t operand_num = 0, byte_offset = 0;
};

std::vector<AsmLine> compile_brainfuck_to_govnoasm(const std::string& bf) {
    std::vector<AsmLine> lines;
    lines.push_back({"_start", "", "", 0, 0});
    std::stack<int> loop_ids;
    int next_id = 0;
    for (size_t i = 0; i < bf.size();) {
        char c = bf[i];
        if (c == '+' || c == '-' || c == '>' || c == '<') {
            uint32_t run = 0;
            while (i < bf.size() && bf[i] == c && run < 250) { ++run; ++i; }
            std::string m = (c == '+') ? "ADD_MEM" : (c == '-') ? "SUB_MEM" : (c == '>') ? "PTR_INC" : "PTR_DEC";
            lines.push_back({"", m, std::to_string(run), run, 0});
        } else if (c == '[') {
            int id = next_id++;
            loop_ids.push(id);
            lines.push_back({"L_OPEN_" + std::to_string(id), "", "", 0, 0});
            lines.push_back({"", "JZ", "L_END_" + std::to_string(id), 0, 0});
            ++i;
        } else if (c == ']') {
            int id = loop_ids.top(); loop_ids.pop();
            lines.push_back({"", "JNZ", "L_OPEN_" + std::to_string(id), 0, 0});
            lines.push_back({"L_END_" + std::to_string(id), "", "", 0, 0});
            ++i;
        } else if (c == '.') {
            lines.push_back({"", "SYSCALL", "1", 1, 0});
            ++i;
        } else { ++i; }
    }
    lines.push_back({"", "HALT", "", 0, 0});
    return lines;
}

uint32_t asm_size(const AsmLine& ln) {
    if (ln.mnemonic.empty()) return 0;
    if (ln.mnemonic == "HALT") return 1;
    if (ln.mnemonic == "JZ" || ln.mnemonic == "JNZ") return 5;
    return 2;
}

std::vector<uint8_t> assemble_and_link_gexe(std::vector<AsmLine>& lines, std::string& out_listing) {
    std::unordered_map<std::string, uint32_t> label_offsets;
    uint32_t offset = 0;
    for (auto& ln : lines) {
        ln.byte_offset = offset;
        if (!ln.label.empty()) label_offsets[ln.label] = offset;
        offset += asm_size(ln);
    }

    std::vector<uint8_t> code;
    code.reserve(offset);
    std::ostringstream lst;
    lst << "; === GOVNO-ASM + GOVNO-LD LINKER SCRIPT ===\r\n.section .text (VMA 0x00401000)\r\n\r\n";

    for (const auto& ln : lines) {
        if (!ln.label.empty()) {
            lst << std::hex << std::setw(4) << std::setfill('0') << ln.byte_offset << "          " << ln.label << ":\r\n";
        }
        if (ln.mnemonic.empty()) continue;
        lst << std::hex << std::setw(4) << std::setfill('0') << ln.byte_offset << "  ";
        if (ln.mnemonic == "ADD_MEM" || ln.mnemonic == "SUB_MEM" ||
            ln.mnemonic == "PTR_INC" || ln.mnemonic == "PTR_DEC" || ln.mnemonic == "SYSCALL") {
            GovnoOpcode op =
                (ln.mnemonic == "ADD_MEM") ? GovnoOpcode::OP_ADD_MEM :
                (ln.mnemonic == "SUB_MEM") ? GovnoOpcode::OP_SUB_MEM :
                (ln.mnemonic == "PTR_INC") ? GovnoOpcode::OP_PTR_INC :
                (ln.mnemonic == "PTR_DEC") ? GovnoOpcode::OP_PTR_DEC : GovnoOpcode::OP_SYSCALL;
            uint8_t imm = static_cast<uint8_t>(ln.operand_num & 0xFF);
            code.push_back(static_cast<uint8_t>(op));
            code.push_back(imm);
            lst << std::setw(2) << (int)static_cast<uint8_t>(op) << " " << std::setw(2) << (int)imm
                << "      " << ln.mnemonic << " " << std::dec << (int)imm << "\r\n";
        } else if (ln.mnemonic == "JZ" || ln.mnemonic == "JNZ") {
            uint8_t op = (ln.mnemonic == "JZ") ? static_cast<uint8_t>(GovnoOpcode::OP_JZ) : static_cast<uint8_t>(GovnoOpcode::OP_JNZ);
            uint32_t target = label_offsets.at(ln.operand_str);
            code.push_back(op);
            for (int s = 0; s < 32; s += 8) code.push_back(static_cast<uint8_t>((target >> s) & 0xFF));
            lst << std::setw(2) << (int)op << " .. ..    " << ln.mnemonic << " " << ln.operand_str << " (@0x" << std::hex << target << ")\r\n";
        } else if (ln.mnemonic == "HALT") {
            code.push_back(static_cast<uint8_t>(GovnoOpcode::OP_HALT));
            lst << "FF          HALT\r\n";
        }
    }

    uint32_t crc = compute_crc32(code.data(), code.size());
    uint32_t code_sz = static_cast<uint32_t>(code.size());
    uint32_t entry = label_offsets.count("_start") ? label_offsets["_start"] : 0u;
    lst << "\r\n; === LINKER MAP ===\r\n0x00401000 _start | size=" << std::dec << code_sz << "B | CRC32=0x" << std::hex << crc << "\r\n";
    out_listing = lst.str();

    std::vector<uint8_t> gexe{'G','E','X','E',0x69,0x00,0x03,0x00};
    for (int s = 0; s < 32; s += 8) gexe.push_back(static_cast<uint8_t>((entry >> s) & 0xFF));
    for (int s = 0; s < 32; s += 8) gexe.push_back(static_cast<uint8_t>((code_sz >> s) & 0xFF));
    gexe.insert(gexe.end(), code.begin(), code.end());
    for (int s = 0; s < 32; s += 8) gexe.push_back(static_cast<uint8_t>((crc >> s) & 0xFF));
    return gexe;
}

} // namespace

std::string format_hex_dump(const std::vector<uint8_t>& bytes) {
    std::ostringstream os;
    os << "=== .GEXE BINARY HEX DUMP (" << bytes.size() << " bytes) ===\r\n";
    for (size_t i = 0; i < bytes.size(); i += 16) {
        os << std::hex << std::setw(4) << std::setfill('0') << i << " : ";
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < bytes.size()) os << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i + j] << ' ';
            else os << "   ";
        }
        os << " | ";
        for (size_t j = 0; j < 16 && i + j < bytes.size(); ++j) {
            uint8_t c = bytes[i + j];
            os << ((c >= 32 && c < 127) ? static_cast<char>(c) : '.');
        }
        os << "\r\n";
    }
    return os.str();
}

bool run_full_compiler_pipeline(
    const std::string& input_payload,
    CompilerArtifacts& out_artifacts,
    const std::function<void(int pct, const std::string& log_msg)>& progress_cb
) {
    NICHEGO_NE_BOYSYA
    const std::string watermark = kCompileTimeWatermark.decrypt_at_runtime_with_suffering();
    progress_cb(5, "[TEMPLATE-ORACLE] " + watermark + " + consteval BF('H')==72 OK");

    // Stage 1: XML -> JSON -> GovnoSQL
    const std::string xml_manifest =
        "<?xml version=\"1.0\"?>\r\n<GovnoEnterpriseConfig>\r\n"
        "  <MagicSalt>13</MagicSalt>\r\n  <Microservice>HelloWorldGateway</Microservice>\r\n"
        "</GovnoEnterpriseConfig>\r\n";
    auto xml_kv = parse_simple_xml_tags(xml_manifest);
    int magic_salt = std::stoi(xml_kv["MagicSalt"]);

    const std::vector<SqlRow> sql_table = {
        {1, "PAGE_RESERVE", 128, 1},
        {2, "OBFUSCATION_BIAS", 7, 1},
        {3, "LEGACY_NOISE", 999, 0}
    };
    size_t scanned = 0;
    auto sql_rows = execute_govnosql_select(sql_table, "SELECT key_name, value FROM sacred_constants WHERE active = 1;", scanned);
    out_artifacts.sql_rows_scanned = scanned;
    std::unordered_map<std::string, int> sql_map;
    for (const auto& r : sql_rows) sql_map[r.key_name] = r.value;

    {
        std::ostringstream xjs;
        xjs << "=== XML ===\r\n" << xml_manifest
            << "\r\n=== JSON ===\r\n{\"service\": \"" << xml_kv["Microservice"] << "\", \"magicSalt\": " << magic_salt << "}\r\n"
            << "\r\n=== GOVNO-SQL (scanned " << scanned << " rows) ===\r\n";
        for (const auto& r : sql_rows) xjs << "  " << r.key_name << " = " << r.value << "\r\n";
        out_artifacts.xml_json_sql_dump = xjs.str();
    }
    progress_cb(16, "[XML->JSON->SQL] Загружен конфиг из XML/JSON и SQL-таблицы (" + std::to_string(scanned) + " строк)");

    // Stage 2: Dynamically synthesize GovnoScript from the Morse/LZ77/RSA/NN/GA recovered string!
    std::ostringstream raw_script;
    int bias = sql_map["OBFUSCATION_BIAS"];
    raw_script << "// Динамически сгенерированный GovnoScript из выхода Нейросети/RSA/Морзе\r\n"
               << "#DEFINE HOLY_SALT " << magic_salt << "\r\n"
               << "#DEFINE SQL_BIAS " << bias << "\r\n"
               << "KERNEL_MODULE \"APOCALYPSE_HELLO_WORLD_V3\";\r\n"
               << "RESERVE_PAGE_FRAMES " << sql_map["PAGE_RESERVE"] << ";\r\n";

    for (size_t idx = 0; idx < input_payload.size(); ++idx) {
        int val = static_cast<unsigned char>(input_payload[idx]);
        int base_part = val + magic_salt - bias;
        std::string vname = "ch_" + std::to_string(idx);
        raw_script << "VAR " << vname << " = (" << base_part << " + SQL_BIAS) - HOLY_SALT;\r\n"
                   << "SYSCALL_EMIT " << vname << ";\r\n";
    }
    raw_script << "HALT_KERNEL_UNIVERSE;\r\n";

    size_t macro_cnt = 0;
    std::string expanded = preprocess_govnoscript(raw_script.str(), macro_cnt);
    out_artifacts.govnoscript_source =
        "=== RAW GOVNOSCRIPT ===\r\n" + raw_script.str() +
        "\r\n=== AFTER #DEFINE (" + std::to_string(macro_cnt) + " macros) ===\r\n" + expanded;
    progress_cb(28, "[PREPROCESSOR] Раскрыто #DEFINE макросов: " + std::to_string(macro_cnt));

    // Stage 3: Goto Lexer
    auto tokens = lex_govnoscript(expanded);
    out_artifacts.token_count = tokens.size();
    {
        std::ostringstream ts;
        for (size_t idx = 0; idx < tokens.size(); ++idx) {
            ts << "[" << idx << "] L" << tokens[idx].line << ":" << tokens[idx].col
               << " " << token_type_name(tokens[idx].type) << " -> '" << tokens[idx].lexeme << "'\r\n";
        }
        out_artifacts.tokens_dump = ts.str();
    }
    progress_cb(40, "[LEXER-GOTO] Нарезано токенов: " + std::to_string(tokens.size()));

    // Stage 4: Parser -> AST
    GovnoParser parser(tokens);
    auto ast = parser.parse_program();
    {
        std::ostringstream as;
        size_t cnt = 0;
        ast->dump(as, 0, cnt);
        out_artifacts.ast_dump = as.str();
        out_artifacts.ast_node_count = cnt;
    }
    progress_cb(52, "[PARSER-AST] Построено AST (" + std::to_string(out_artifacts.ast_node_count) + " узлов)");

    // Stage 5: Deoptimizer + Dead-Code IR
    IRBuilder ir_builder;
    std::vector<IRInstruction> ir;
    std::vector<uint8_t> emitted_bytes = ir_builder.compile_ast_to_ir(ast, ir);
    out_artifacts.ir_instruction_count = ir.size();
    {
        std::ostringstream irs;
        for (size_t k = 0; k < ir.size(); ++k) {
            irs << std::setw(3) << k << ": " << ir[k].op << " " << ir[k].dst;
            if (!ir[k].src1.empty()) irs << ", " << ir[k].src1;
            if (!ir[k].src2.empty()) irs << ", " << ir[k].src2;
            irs << "  [val=" << ir[k].imm << "]\r\n";
        }
        out_artifacts.ir_dump = irs.str();
    }
    progress_cb(64, "[DEOPTIMIZER-IR] Опущено в 3-адресный IR (" + std::to_string(ir.size()) + " команд)");

    // Stage 6: GovnoLisp -> C99 -> Brainfuck -> Peephole -> Verify
    auto [lisp_c, lisp_bytes] = compile_bytes_to_lisp_and_c(
        emitted_bytes, out_artifacts.lisp_forms_count, out_artifacts.c_lines_count
    );
    out_artifacts.lisp_and_c_dump = lisp_c;
    progress_cb(75, "[LISP->C99] Сгенерирован Lisp и C99 код (" + std::to_string(out_artifacts.c_lines_count) + " строк)");

    std::string opt_bf = optimize_brainfuck_peephole(generate_brainfuck_from_bytes(lisp_bytes));
    std::string bf_check = verify_brainfuck_execution(opt_bf);
    out_artifacts.brainfuck_code = opt_bf;
    out_artifacts.bf_length = opt_bf.size();
    out_artifacts.brainfuck_verified_output = bf_check;
    progress_cb(84, "[BRAINFUCK] Сгенерирован и проверен Brainfuck (" + std::to_string(opt_bf.size()) + " симв.)");

    // Stage 7: SUBLEQ One-Instruction Set Computer (OISC) compilation + execution!
    std::vector<uint8_t> verified_bytes(bf_check.begin(), bf_check.end());
    auto subleq = govno::madness::compile_and_execute_subleq_oisc(verified_bytes);
    out_artifacts.subleq_oisc_dump = subleq.disassembly_dump;
    out_artifacts.subleq_cycles = subleq.cycles_executed;
    progress_cb(92, "[SUBLEQ-OISC] Прогнано через процессор с 1 инструкцией (" + std::to_string(subleq.cycles_executed) + " тактов) -> \"" + subleq.output + "\"");

    // Stage 8: GovnoASM -> GovnoLD Linker -> .gexe binary
    std::vector<uint8_t> final_bytes(subleq.output.begin(), subleq.output.end());
    std::string final_bf = optimize_brainfuck_peephole(generate_brainfuck_from_bytes(final_bytes));
    auto asm_lines = compile_brainfuck_to_govnoasm(final_bf);
    out_artifacts.asm_line_count = asm_lines.size();
    std::string listing_and_map;
    auto gexe = assemble_and_link_gexe(asm_lines, listing_and_map);
    out_artifacts.govnoasm_and_linker_dump = listing_and_map;
    out_artifacts.gexe_bytecode = gexe;
    out_artifacts.gexe_size_bytes = gexe.size();
    progress_cb(100, "[GOVNO-LD] Слинкован /bin/hello_world.gexe (" + std::to_string(gexe.size()) + " байт, CRC32 OK)");

    return true;
    LOVI_APOKALIPSIS
    progress_cb(0, "[COMPILER-PANIC] Неизвестная ошибка компиляции!");
    return false;
    PROSTO_ZABEY
}

} // namespace govno::compiler
