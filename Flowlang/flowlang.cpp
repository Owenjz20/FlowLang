// flowlang.cpp
// FlowLang: interpreter + AI stub + local package manager
// Single-file, portable C++17 core (no OS-specific headers here)

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cctype>
#include <cstdlib>

// =======================
// LEXER
// =======================

enum class TokenType {
    Identifier,
    Number,
    String,
    KeywordLet,
    KeywordFunc,
    KeywordReturn,
    KeywordIf,
    KeywordElse,
    KeywordFor,
    KeywordIn,
    KeywordPrint,
    LParen, RParen,
    LBrace, RBrace,
    Comma,
    Assign,
    Plus, Minus, Star, Slash,
    Equal, NotEqual,
    Less, LessEqual,
    Greater, GreaterEqual,
    DotDot,
    EndOfFile,
    Unknown
};

struct Token {
    TokenType type;
    std::string text;
    int line;
    int col;
};

class Lexer {
public:
    explicit Lexer(const std::string& src)
        : src(src), pos(0), line(1), col(1) {}

    Token next() {
        skipWhitespaceAndComments();
        if (pos >= src.size()) return makeToken(TokenType::EndOfFile, "");

        char c = src[pos];

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') return identifier();
        if (std::isdigit(static_cast<unsigned char>(c))) return number();
        if (c == '"') return string();

        switch (c) {
            case '(': return simple(TokenType::LParen, "(");
            case ')': return simple(TokenType::RParen, ")");
            case '{': return simple(TokenType::LBrace, "{");
            case '}': return simple(TokenType::RBrace, "}");
            case ',': return simple(TokenType::Comma, ",");
            case '+': return simple(TokenType::Plus, "+");
            case '-': return simple(TokenType::Minus, "-");
            case '*': return simple(TokenType::Star, "*");
            case '/': return simple(TokenType::Slash, "/");
            case '=':
                if (peek() == '=') { advance(); return simple(TokenType::Equal, "=="); }
                return simple(TokenType::Assign, "=");
            case '!':
                if (peek() == '=') { advance(); return simple(TokenType::NotEqual, "!="); }
                break;
            case '<':
                if (peek() == '=') { advance(); return simple(TokenType::LessEqual, "<="); }
                return simple(TokenType::Less, "<");
            case '>':
                if (peek() == '=') { advance(); return simple(TokenType::GreaterEqual, ">="); }
                return simple(TokenType::Greater, ">");
            case '.':
                if (peek() == '.') { advance(); return simple(TokenType::DotDot, ".."); }
                break;
        }

        return simple(TokenType::Unknown, std::string(1, c));
    }

private:
    std::string src;
    size_t pos;
    int line, col;

    char peek() const {
        if (pos + 1 >= src.size()) return '\0';
        return src[pos + 1];
    }

    char advance() {
        char c = src[pos++];
        if (c == '\n') { line++; col = 1; }
        else col++;
        return c;
    }

    void skipWhitespaceAndComments() {
        while (pos < src.size()) {
            char c = src[pos];
            if (std::isspace(static_cast<unsigned char>(c))) { advance(); continue; }
            if (c == '/' && peek() == '/') {
                while (pos < src.size() && src[pos] != '\n') advance();
                continue;
            }
            break;
        }
    }

    Token makeToken(TokenType type, const std::string& text) {
        return Token{ type, text, line, col };
    }

    Token simple(TokenType type, const std::string& text) {
        advance();
        return makeToken(type, text);
    }

    Token identifier() {
        int startCol = col;
        std::string text;
        while (pos < src.size() &&
               (std::isalnum(static_cast<unsigned char>(src[pos])) || src[pos] == '_')) {
            text.push_back(advance());
        }
        if (text == "let")    return Token{ TokenType::KeywordLet,    text, line, startCol };
        if (text == "func")   return Token{ TokenType::KeywordFunc,   text, line, startCol };
        if (text == "return") return Token{ TokenType::KeywordReturn, text, line, startCol };
        if (text == "if")     return Token{ TokenType::KeywordIf,     text, line, startCol };
        if (text == "else")   return Token{ TokenType::KeywordElse,   text, line, startCol };
        if (text == "for")    return Token{ TokenType::KeywordFor,    text, line, startCol };
        if (text == "in")     return Token{ TokenType::KeywordIn,     text, line, startCol };
        if (text == "print")  return Token{ TokenType::KeywordPrint,  text, line, startCol };
        return Token{ TokenType::Identifier, text, line, startCol };
    }

    Token number() {
        int startCol = col;
        std::string text;
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
            text.push_back(advance());
        }
        return Token{ TokenType::Number, text, line, startCol };
    }

    Token string() {
        int startCol = col;
        advance(); // skip "
        std::string text;
        while (pos < src.size() && src[pos] != '"') {
            text.push_back(advance());
        }
        if (pos < src.size()) advance(); // closing "
        return Token{ TokenType::String, text, line, startCol };
    }
};

// =======================
// AST
// =======================

struct Expr;
struct Stmt;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

enum class ExprKind {
    Literal,
    Variable,
    Binary,
    Call
};

enum class StmtKind {
    Let,
    ExprStmt,
    Print,
    Block,
    If,
    For,
    Func,
    Return
};

struct Expr {
    ExprKind kind;
    std::string value;           // literal text, variable name, operator, function name
    ExprPtr left;
    ExprPtr right;
    std::vector<ExprPtr> args;   // for Call
};

struct Stmt {
    StmtKind kind;
    std::string name;            // var name, func name
    ExprPtr expr;                // expression or return value
    ExprPtr init;                // for loops: start
    ExprPtr cond;                // for loops: end / if: condition
    std::vector<std::string> params; // func params
    std::vector<StmtPtr> body;
    std::vector<StmtPtr> elseBody;
};

// =======================
// PARSER
// =======================

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens(std::move(tokens)), pos(0) {}

    std::vector<StmtPtr> parse() {
        std::vector<StmtPtr> stmts;
        while (!check(TokenType::EndOfFile)) {
            stmts.push_back(declaration());
        }
        return stmts;
    }

private:
    std::vector<Token> tokens;
    size_t pos;

    bool match(TokenType type) {
        if (check(type)) { advance(); return true; }
        return false;
    }

    bool check(TokenType type) const {
        if (pos >= tokens.size()) return false;
        return tokens[pos].type == type;
    }

    const Token& advance() {
        return tokens[pos++];
    }

    const Token& peek() const {
        return tokens[pos];
    }

    StmtPtr declaration() {
        if (match(TokenType::KeywordFunc)) return funcDeclaration();
        return statement();
    }

    StmtPtr funcDeclaration() {
        Token name = advance(); // identifier
        match(TokenType::LParen);
        std::vector<std::string> params;
        if (!check(TokenType::RParen)) {
            do {
                Token p = advance();
                params.push_back(p.text);
            } while (match(TokenType::Comma));
        }
        match(TokenType::RParen);
        auto body = block();

        auto stmt = std::make_shared<Stmt>();
        stmt->kind = StmtKind::Func;
        stmt->name = name.text;
        stmt->params = params;
        stmt->body = body;
        return stmt;
    }

    StmtPtr statement() {
        if (match(TokenType::KeywordLet))   return letStatement();
        if (match(TokenType::KeywordPrint)) return printStatement();
        if (match(TokenType::KeywordIf))    return ifStatement();
        if (match(TokenType::KeywordFor))   return forStatement();
        if (match(TokenType::KeywordReturn)) return returnStatement();
        return exprStatement();
    }

    StmtPtr letStatement() {
        Token name = advance(); // identifier
        match(TokenType::Assign);
        ExprPtr value = expression();
        auto stmt = std::make_shared<Stmt>();
        stmt->kind = StmtKind::Let;
        stmt->name = name.text;
        stmt->expr = value;
        return stmt;
    }

    StmtPtr printStatement() {
        ExprPtr value = expression();
        auto stmt = std::make_shared<Stmt>();
        stmt->kind = StmtKind::Print;
        stmt->expr = value;
        return stmt;
    }

    StmtPtr ifStatement() {
        ExprPtr cond = expression();
        auto thenBlock = block();
        std::vector<StmtPtr> elseBlock;
        if (match(TokenType::KeywordElse)) {
            elseBlock = block();
        }
        auto stmt = std::make_shared<Stmt>();
        stmt->kind = StmtKind::If;
        stmt->cond = cond;
        stmt->body = thenBlock;
        stmt->elseBody = elseBlock;
        return stmt;
    }

    StmtPtr forStatement() {
        // for i in 1..5 { ... }
        Token name = advance(); // identifier
        match(TokenType::KeywordIn);
        ExprPtr start = expression();
        match(TokenType::DotDot);
        ExprPtr end = expression();
        auto bodyBlock = block();

        auto stmt = std::make_shared<Stmt>();
        stmt->kind = StmtKind::For;
        stmt->name = name.text;
        stmt->init = start;
        stmt->cond = end;
        stmt->body = bodyBlock;
        return stmt;
    }

    StmtPtr returnStatement() {
        ExprPtr value = nullptr;
        if (!check(TokenType::EndOfFile) && !check(TokenType::RBrace)) {
            value = expression();
        }
        auto stmt = std::make_shared<Stmt>();
        stmt->kind = StmtKind::Return;
        stmt->expr = value;
        return stmt;
    }

    std::vector<StmtPtr> block() {
        std::vector<StmtPtr> stmts;
        if (!match(TokenType::LBrace)) return stmts;
        while (!check(TokenType::RBrace) && !check(TokenType::EndOfFile)) {
            stmts.push_back(declaration());
        }
        match(TokenType::RBrace);
        return stmts;
    }

    StmtPtr exprStatement() {
        ExprPtr e = expression();
        auto stmt = std::make_shared<Stmt>();
        stmt->kind = StmtKind::ExprStmt;
        stmt->expr = e;
        return stmt;
    }

    ExprPtr expression() {
        return equality();
    }

    ExprPtr equality() {
        ExprPtr expr = comparison();
        while (match(TokenType::Equal) || match(TokenType::NotEqual)) {
            Token op = tokens[pos - 1];
            ExprPtr right = comparison();
            expr = makeBinary(expr, op.text, right);
        }
        return expr;
    }

    ExprPtr comparison() {
        ExprPtr expr = term();
        while (match(TokenType::Less) || match(TokenType::LessEqual) ||
               match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
            Token op = tokens[pos - 1];
            ExprPtr right = term();
            expr = makeBinary(expr, op.text, right);
        }
        return expr;
    }

    ExprPtr term() {
        ExprPtr expr = factor();
        while (match(TokenType::Plus) || match(TokenType::Minus)) {
            Token op = tokens[pos - 1];
            ExprPtr right = factor();
            expr = makeBinary(expr, op.text, right);
        }
        return expr;
    }

    ExprPtr factor() {
        ExprPtr expr = unary();
        while (match(TokenType::Star) || match(TokenType::Slash)) {
            Token op = tokens[pos - 1];
            ExprPtr right = unary();
            expr = makeBinary(expr, op.text, right);
        }
        return expr;
    }

    ExprPtr unary() {
        // no unary operators yet, just primary
        return call();
    }

    ExprPtr call() {
        ExprPtr expr = primary();
        while (match(TokenType::LParen)) {
            std::vector<ExprPtr> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(expression());
                } while (match(TokenType::Comma));
            }
            match(TokenType::RParen);
            auto callExpr = std::make_shared<Expr>();
            callExpr->kind = ExprKind::Call;
            callExpr->value = expr->value; // function name
            callExpr->args = args;
            expr = callExpr;
        }
        return expr;
    }

    ExprPtr primary() {
        if (match(TokenType::Number) || match(TokenType::String)) {
            Token t = tokens[pos - 1];
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Literal;
            e->value = t.text;
            return e;
        }
        if (match(TokenType::Identifier)) {
            Token t = tokens[pos - 1];
            auto e = std::make_shared<Expr>();
            e->kind = ExprKind::Variable;
            e->value = t.text;
            return e;
        }
        if (match(TokenType::LParen)) {
            ExprPtr e = expression();
            match(TokenType::RParen);
            return e;
        }
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Literal;
        e->value = "0";
        return e;
    }

    ExprPtr makeBinary(ExprPtr left, const std::string& op, ExprPtr right) {
        auto e = std::make_shared<Expr>();
        e->kind = ExprKind::Binary;
        e->value = op;
        e->left = left;
        e->right = right;
        return e;
    }
};

// =======================
// RUNTIME
// =======================

struct Value {
    enum class Type { Number, String, None } type;
    double num;
    std::string str;

    static Value number(double n) { return Value{ Type::Number, n, "" }; }
    static Value string(const std::string& s) { return Value{ Type::String, 0.0, s }; }
    static Value none() { return Value{ Type::None, 0.0, "" }; }
};

struct Function {
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
};

class Env {
public:
    Env* parent = nullptr;

    Value get(const std::string& name) {
        if (vars.count(name)) return vars[name];
        if (parent) return parent->get(name);
        return Value::none();
    }

    void set(const std::string& name, const Value& v) {
        vars[name] = v;
    }

    bool hasLocal(const std::string& name) const {
        return vars.count(name) > 0;
    }

private:
    std::unordered_map<std::string, Value> vars;
};

class Interpreter {
public:
    Interpreter() {
        // basic std lib
        // std.print is handled as a special built-in
        // std.len is also built-in
    }

    void addFunction(const std::string& name, const Function& fn) {
        functions[name] = fn;
    }

    void run(const std::vector<StmtPtr>& stmts) {
        globalEnv = std::make_shared<Env>();
        execBlock(stmts, globalEnv.get());
    }

private:
    std::unordered_map<std::string, Function> functions;
    std::shared_ptr<Env> globalEnv;

    struct ReturnSignal {
        bool hasValue = false;
        Value value;
    };

    void execBlock(const std::vector<StmtPtr>& body, Env* env, ReturnSignal* retSig = nullptr) {
        for (auto& s : body) {
            exec(s, env, retSig);
            if (retSig && retSig->hasValue) return;
        }
    }

    void exec(const StmtPtr& s, Env* env, ReturnSignal* retSig = nullptr) {
        switch (s->kind) {
            case StmtKind::Let:      execLet(s, env); break;
            case StmtKind::Print:    execPrint(s, env); break;
            case StmtKind::ExprStmt: eval(s->expr, env); break;
            case StmtKind::If:       execIf(s, env, retSig); break;
            case StmtKind::For:      execFor(s, env, retSig); break;
            case StmtKind::Func:     execFunc(s); break;
            case StmtKind::Return:   execReturn(s, env, retSig); break;
            case StmtKind::Block:    execBlock(s->body, env, retSig); break;
        }
    }

    void execLet(const StmtPtr& s, Env* env) {
        Value v = eval(s->expr, env);
        env->set(s->name, v);
    }

    void execPrint(const StmtPtr& s, Env* env) {
        Value v = eval(s->expr, env);
        if (v.type == Value::Type::Number) std::cout << v.num << "\n";
        else if (v.type == Value::Type::String) std::cout << v.str << "\n";
        else std::cout << "none\n";
    }

    void execIf(const StmtPtr& s, Env* env, ReturnSignal* retSig) {
        Value c = eval(s->cond, env);
        bool cond = (c.type == Value::Type::Number && c.num != 0);
        if (cond) {
            execBlock(s->body, env, retSig);
        } else {
            execBlock(s->elseBody, env, retSig);
        }
    }

    void execFor(const StmtPtr& s, Env* env, ReturnSignal* retSig) {
        Value start = eval(s->init, env);
        Value end = eval(s->cond, env);
        int a = (int)start.num;
        int b = (int)end.num;
        for (int i = a; i <= b; ++i) {
            env->set(s->name, Value::number(i));
            execBlock(s->body, env, retSig);
            if (retSig && retSig->hasValue) return;
        }
    }

    void execFunc(const StmtPtr& s) {
        Function fn;
        fn.params = s->params;
        fn.body = s->body;
        functions[s->name] = fn;
    }

    void execReturn(const StmtPtr& s, Env* env, ReturnSignal* retSig) {
        if (!retSig) return;
        if (s->expr) retSig->value = eval(s->expr, env);
        else retSig->value = Value::none();
        retSig->hasValue = true;
    }

    Value eval(const ExprPtr& e, Env* env) {
        switch (e->kind) {
            case ExprKind::Literal:
                if (!e->value.empty() && std::isdigit(static_cast<unsigned char>(e->value[0])))
                    return Value::number(std::stod(e->value));
                return Value::string(e->value);
            case ExprKind::Variable:
                return env->get(e->value);
            case ExprKind::Binary:
                return evalBinary(e, env);
            case ExprKind::Call:
                return evalCall(e, env);
        }
        return Value::none();
    }

    Value evalBinary(const ExprPtr& e, Env* env) {
        Value l = eval(e->left, env);
        Value r = eval(e->right, env);
        if (l.type == Value::Type::Number && r.type == Value::Type::Number) {
            if (e->value == "+")  return Value::number(l.num + r.num);
            if (e->value == "-")  return Value::number(l.num - r.num);
            if (e->value == "*")  return Value::number(l.num * r.num);
            if (e->value == "/")  return Value::number(l.num / r.num);
            if (e->value == "==") return Value::number(l.num == r.num);
            if (e->value == "!=") return Value::number(l.num != r.num);
            if (e->value == "<")  return Value::number(l.num < r.num);
            if (e->value == "<=") return Value::number(l.num <= r.num);
            if (e->value == ">")  return Value::number(l.num > r.num);
            if (e->value == ">=") return Value::number(l.num >= r.num);
        }
        return Value::none();
    }

    Value evalCall(const ExprPtr& e, Env* env) {
        // built-ins
        if (e->value == "std.print") {
            for (auto& a : e->args) {
                Value v = eval(a, env);
                if (v.type == Value::Type::Number) std::cout << v.num;
                else if (v.type == Value::Type::String) std::cout << v.str;
            }
            std::cout << "\n";
            return Value::none();
        }
        if (e->value == "std.len") {
            if (e->args.empty()) return Value::number(0);
            Value v = eval(e->args[0], env);
            if (v.type == Value::Type::String) return Value::number((double)v.str.size());
            return Value::number(0);
        }

        // user-defined
        if (!functions.count(e->value)) {
            std::cerr << "Unknown function: " << e->value << "\n";
            return Value::none();
        }
        Function& fn = functions[e->value];
        auto local = std::make_shared<Env>();
        local->parent = env;
        for (size_t i = 0; i < fn.params.size() && i < e->args.size(); ++i) {
            Value v = eval(e->args[i], env);
            local->set(fn.params[i], v);
        }
        ReturnSignal ret;
        execBlock(fn.body, local.get(), &ret);
        if (ret.hasValue) return ret.value;
        return Value::none();
    }
};

// =======================
// AI STUB
// =======================

std::string ai_generate_flowlang(const std::string& prompt) {
    if (prompt.find("loop") != std::string::npos) {
        return "for i in 1..5 {\n    print(i)\n}";
    }
    if (prompt.find("add") != std::string::npos) {
        return "let a = 1\nlet b = 2\nprint(a + b)";
    }
    if (prompt.find("func") != std::string::npos) {
        return "func add(a, b) {\n    return a + b\n}\nprint(add(2, 3))";
    }
    return "print(\"FlowLang AI placeholder\")";
}

// =======================
// PACKAGE MANAGER (local)
// =======================

#ifdef _WIN32
#define MKDIR_CMD "mkdir "
#define RMDIR_CMD "rmdir /S /Q "
#define LS_CMD    "dir"
#else
#define MKDIR_CMD "mkdir -p "
#define RMDIR_CMD "rm -rf "
#define LS_CMD    "ls"
#endif

void pkg_install_local(const std::string& name, const std::string& path) {
    std::string base = "flowlang_packages/";
    std::string cmd = std::string(MKDIR_CMD) + base + name;
    system(cmd.c_str());

    std::ifstream in(path);
    if (!in) {
        std::cerr << "Could not read package file: " << path << "\n";
        return;
    }
    std::ofstream out(base + name + "/package.flow");
    out << in.rdbuf();
    std::cout << "Installed local package: " << name << "\n";
}

void pkg_remove(const std::string& name) {
    std::string cmd = std::string(RMDIR_CMD) + "flowlang_packages/" + name;
    system(cmd.c_str());
    std::cout << "Removed package: " << name << "\n";
}

void pkg_list() {
    std::cout << "Installed packages:\n";
    std::string cmd = std::string(LS_CMD) + " flowlang_packages";
    system(cmd.c_str());
}

// =======================
// UTIL
// =======================

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) return "";
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// =======================
// MAIN
// =======================

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "FlowLang Interpreter\n";
        std::cout << "Usage:\n";
        std::cout << "  flowlang <file.flow>\n";
        std::cout << "  flowlang --ai \"prompt\"\n";
        std::cout << "  flowlang pkg install-local <name> <file.flow>\n";
        std::cout << "  flowlang pkg remove <name>\n";
        std::cout << "  flowlang pkg list\n";
        return 0;
    }

    std::string arg1 = argv[1];

    // AI mode
    if (arg1 == "--ai") {
        if (argc < 3) {
            std::cerr << "Missing prompt.\n";
            return 1;
        }
        std::string prompt = argv[2];
        std::string code = ai_generate_flowlang(prompt);
        std::cout << code << "\n";
        return 0;
    }

    // Package manager
    if (arg1 == "pkg") {
        if (argc < 3) {
            std::cerr << "Missing pkg command.\n";
            return 1;
        }
        std::string cmd = argv[2];
        if (cmd == "install-local") {
            if (argc < 5) {
                std::cerr << "Usage: flowlang pkg install-local <name> <file.flow>\n";
                return 1;
            }
            pkg_install_local(argv[3], argv[4]);
        } else if (cmd == "remove") {
            if (argc < 4) {
                std::cerr << "Usage: flowlang pkg remove <name>\n";
                return 1;
            }
            pkg_remove(argv[3]);
        } else if (cmd == "list") {
            pkg_list();
        } else {
            std::cerr << "Unknown pkg command.\n";
        }
        return 0;
    }

    // Normal script execution
    std::string src = read_file(arg1);
    if (src.empty()) {
        std::cerr << "Could not read file: " << arg1 << "\n";
        return 1;
    }

    Lexer lex(src);
    std::vector<Token> tokens;
    while (true) {
        Token t = lex.next();
        tokens.push_back(t);
        if (t.type == TokenType::EndOfFile) break;
    }

    Parser parser(tokens);
    auto stmts = parser.parse();

    Interpreter interp;
    interp.run(stmts);

    return 0;
}
