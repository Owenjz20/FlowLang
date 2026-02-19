#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include <curl/curl.h>
#include "llama.h"

namespace fs = std::filesystem;

// ======================================================
// Utility
// ======================================================

static void die(const std::string & msg) {
    std::cerr << "[flowlang] error: " << msg << std::endl;
    std::exit(1);
}

// ======================================================
// Llama: manual greedy sampler (your DLL has logits, no sampler)
// ======================================================

static int greedy_sample(llama_context * ctx, llama_model * model) {
    const float * logits = llama_get_logits(ctx);
    if (!logits) die("llama_get_logits returned null");

    int vocab = llama_n_vocab(model);
    if (vocab <= 0) die("invalid vocab size");

    int best = 0;
    float best_logit = logits[0];

    for (int i = 1; i < vocab; i++) {
        if (logits[i] > best_logit) {
            best_logit = logits[i];
            best = i;
        }
    }

    return best;
}

static std::string llm_generate(const std::string & model_path,
                                const std::string & prompt,
                                int max_tokens = 256) {
    llama_backend_init();

    llama_model_params mparams = llama_model_default_params();
    llama_model * model = llama_load_model_from_file(model_path.c_str(), &mparams);
    if (!model) die("Failed to load model");

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 2048;
    cparams.n_threads = 4;
    cparams.embedding = false;

    llama_context * ctx = llama_new_context_with_model(model, &cparams);
    if (!ctx) die("Failed to create context");

    std::vector<int32_t> tokens(512);
    int n = llama_tokenize(model, prompt.c_str(), (int)prompt.size(),
                           tokens.data(), (int)tokens.size(), true);
    if (n < 0) die("Tokenization failed");
    if (n > 512) n = 512;
    tokens.resize(n);

    llama_batch batch = llama_batch_init(512, 0, 1);

    for (int i = 0; i < n; i++) {
        batch.token[i] = tokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
    }
    batch.n_tokens = n;

    if (llama_decode(ctx, batch) != 0) die("Initial decode failed");

    std::string out;
    out.reserve(max_tokens * 4);

    int eos = llama_token_eos(model);

    for (int i = 0; i < max_tokens; i++) {
        int tok = greedy_sample(ctx, model);
        if (tok == eos) break;

        char buf[256];
        int len = llama_token_to_piece(model, tok, buf, sizeof(buf), true);
        if (len > 0) out.append(buf, len);

        batch.token[0] = tok;
        batch.pos[0] = n + i;
        batch.n_tokens = 1;
        batch.n_seq_id[0] = 1;
        batch.seq_id[0][0] = 0;

        if (llama_decode(ctx, batch) != 0) break;
    }

    llama_batch_free(batch);
    llama_free(ctx);
    llama_free_model(model);
    llama_backend_free();

    return out;
}

// ======================================================
// IDE + LLM REPL
// ======================================================

static std::string build_ide_prompt(const std::string & spec) {
    std::ostringstream ss;
    ss << "<s>[INSTRUCTION]\n";
    ss << "You are FlowLang, a code generator for the FlowLang language.\n";
    ss << "Given a natural language description, output ONLY valid FlowLang code.\n\n";
    ss << "[INSTRUCTION]\n" << spec << "\n\n";
    ss << "[OUTPUT]\n";
    return ss.str();
}

static void cmd_ide(const std::string & model_path) {
    std::cout << "FlowLang IDE\n";
    std::cout << "Model: " << model_path << "\n";
    std::cout << "Enter description, then press ENTER twice.\n\n";

    while (true) {
        std::cout << ">>> ";
        std::string line, spec;
        while (std::getline(std::cin, line)) {
            if (line.empty()) break;
            spec += line + "\n";
        }
        if (spec.empty()) break;

        std::string prompt = build_ide_prompt(spec);
        std::string code = llm_generate(model_path, prompt);

        std::cout << "\n[FlowLang Output]\n" << code << "\n\n";
    }
}

static void cmd_llm(const std::string & prompt,
                    const std::string & model_path,
                    int max_tokens) {
    std::string out = llm_generate(model_path, prompt, max_tokens);
    std::cout << out << std::endl;
}

static void cmd_repl_llm(const std::string & model_path) {
    std::cout << "FlowLang REPL (LLM-backed)\n";
    std::cout << "Model: " << model_path << "\n";
    std::cout << "Type 'exit' to quit.\n\n";

    std::string line;
    while (true) {
        std::cout << "llm> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;
        if (line.empty()) continue;

        std::string out = llm_generate(model_path, line, 128);
        std::cout << out << "\n\n";
    }
}

// ======================================================
// Syntax highlighting (Python-like FlowLang)
// ======================================================

static bool is_identifier_start(char c) {
    return std::isalpha((unsigned char)c) || c == '_';
}

static bool is_identifier_char(char c) {
    return std::isalnum((unsigned char)c) || c == '_';
}

static bool is_keyword(const std::string & w) {
    static const char * kw[] = {
        "let", "def", "return", "if", "else", "while", "True", "False", "None", "print"
    };
    for (auto k : kw) {
        if (w == k) return true;
    }
    return false;
}

static void highlight_flowlang(const std::string & code) {
    const std::string RESET = "\x1b[0m";
    const std::string KW    = "\x1b[38;5;81m";
    const std::string STR   = "\x1b[38;5;214m";
    const std::string NUM   = "\x1b[38;5;142m";
    const std::string COMM  = "\x1b[38;5;240m";

    for (size_t i = 0; i < code.size();) {
        char c = code[i];

        if (c == '#') {
            std::cout << COMM;
            while (i < code.size() && code[i] != '\n') {
                std::cout << code[i++];
            }
            std::cout << RESET;
            continue;
        }

        if (c == '"' || c == '\'') {
            char quote = c;
            std::cout << STR << c;
            i++;
            while (i < code.size()) {
                char d = code[i];
                std::cout << d;
                i++;
                if (d == '\\' && i < code.size()) {
                    std::cout << code[i];
                    i++;
                } else if (d == quote) {
                    break;
                }
            }
            std::cout << RESET;
            continue;
        }

        if (std::isdigit((unsigned char)c)) {
            std::cout << NUM;
            while (i < code.size() && (std::isdigit((unsigned char)code[i]) || code[i] == '.')) {
                std::cout << code[i++];
            }
            std::cout << RESET;
            continue;
        }

        if (is_identifier_start(c)) {
            std::string w;
            size_t j = i;
            while (j < code.size() && is_identifier_char(code[j])) {
                w.push_back(code[j]);
                j++;
            }
            if (is_keyword(w)) {
                std::cout << KW << w << RESET;
            } else {
                std::cout << w;
            }
            i = j;
            continue;
        }

        std::cout << c;
        i++;
    }
}

// ======================================================
// Minimal Python-like FlowLang interpreter
// ======================================================

struct Value {
    enum Kind { INT, STR, NONE } kind;
    long long i;
    std::string s;

    Value() : kind(NONE), i(0) {}
    static Value make_int(long long v) { Value x; x.kind = INT; x.i = v; return x; }
    static Value make_str(const std::string & v) { Value x; x.kind = STR; x.s = v; return x; }
    static Value make_none() { return Value(); }

    std::string repr() const {
        if (kind == INT) return std::to_string(i);
        if (kind == STR) return "\"" + s + "\"";
        return "None";
    }
};

struct Env {
    std::unordered_map<std::string, Value> vars;
};

static std::string trim(const std::string & s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) a++;
    while (b > a && std::isspace((unsigned char)s[b-1])) b--;
    return s.substr(a, b-a);
}

// very small expression parser: integers, strings, + - * /, variables, print args
// grammar (loose):
// expr  := term (('+'|'-') term)*
// term  := factor (('*'|'/') factor)*
// factor:= INT | STRING | IDENT | '(' expr ')'

static size_t expr_pos;
static std::string expr_src;
static Env * expr_env;

static void skip_ws() {
    while (expr_pos < expr_src.size() && std::isspace((unsigned char)expr_src[expr_pos])) expr_pos++;
}

static bool match_char(char c) {
    skip_ws();
    if (expr_pos < expr_src.size() && expr_src[expr_pos] == c) {
        expr_pos++;
        return true;
    }
    return false;
}

static Value parse_expr(); // fwd

static Value parse_factor() {
    skip_ws();
    if (expr_pos >= expr_src.size()) return Value::make_none();
    char c = expr_src[expr_pos];

    if (c == '(') {
        expr_pos++;
        Value v = parse_expr();
        match_char(')');
        return v;
    }

    if (std::isdigit((unsigned char)c)) {
        long long v = 0;
        while (expr_pos < expr_src.size() && std::isdigit((unsigned char)expr_src[expr_pos])) {
            v = v * 10 + (expr_src[expr_pos] - '0');
            expr_pos++;
        }
        return Value::make_int(v);
    }

    if (c == '"' || c == '\'') {
        char quote = c;
        expr_pos++;
        std::string s;
        while (expr_pos < expr_src.size()) {
            char d = expr_src[expr_pos++];
            if (d == '\\' && expr_pos < expr_src.size()) {
                char e = expr_src[expr_pos++];
                s.push_back(e);
            } else if (d == quote) {
                break;
            } else {
                s.push_back(d);
            }
        }
        return Value::make_str(s);
    }

    if (is_identifier_start(c)) {
        std::string name;
        while (expr_pos < expr_src.size() && is_identifier_char(expr_src[expr_pos])) {
            name.push_back(expr_src[expr_pos++]);
        }
        auto it = expr_env->vars.find(name);
        if (it != expr_env->vars.end()) return it->second;
        if (name == "None") return Value::make_none();
        if (name == "True") return Value::make_int(1);
        if (name == "False") return Value::make_int(0);
        return Value::make_none();
    }

    expr_pos++;
    return Value::make_none();
}

static Value parse_term() {
    Value v = parse_factor();
    while (true) {
        skip_ws();
        if (expr_pos >= expr_src.size()) break;
        char c = expr_src[expr_pos];
        if (c != '*' && c != '/') break;
        expr_pos++;
        Value rhs = parse_factor();
        if (v.kind == Value::INT && rhs.kind == Value::INT) {
            if (c == '*') v.i = v.i * rhs.i;
            else if (c == '/') v.i = rhs.i != 0 ? v.i / rhs.i : 0;
        }
    }
    return v;
}

static Value parse_expr() {
    Value v = parse_term();
    while (true) {
        skip_ws();
        if (expr_pos >= expr_src.size()) break;
        char c = expr_src[expr_pos];
        if (c != '+' && c != '-') break;
        expr_pos++;
        Value rhs = parse_term();
        if (v.kind == Value::INT && rhs.kind == Value::INT) {
            if (c == '+') v.i = v.i + rhs.i;
            else v.i = v.i - rhs.i;
        } else if (v.kind == Value::STR || rhs.kind == Value::STR) {
            std::string ls = (v.kind == Value::STR) ? v.s : v.repr();
            std::string rs = (rhs.kind == Value::STR) ? rhs.s : rhs.repr();
            if (c == '+') v = Value::make_str(ls + rs);
        }
    }
    return v;
}

static Value eval_expr(const std::string & src, Env & env) {
    expr_src = src;
    expr_pos = 0;
    expr_env = &env;
    return parse_expr();
}

// execute a single line of FlowLang (Python-like, very small subset)
// supported:
//   name = expr
//   print(expr)
//   bare expr (evaluated, result printed)

static void exec_line(const std::string & line, Env & env) {
    std::string s = trim(line);
    if (s.empty() || s[0] == '#') return;

    if (s.rfind("print", 0) == 0) {
        size_t p = s.find('(');
        size_t q = s.rfind(')');
        if (p != std::string::npos && q != std::string::npos && q > p) {
            std::string inside = s.substr(p+1, q-p-1);
            Value v = eval_expr(inside, env);
            std::cout << v.repr() << std::endl;
            return;
        }
    }

    size_t eq = s.find('=');
    if (eq != std::string::npos) {
        std::string lhs = trim(s.substr(0, eq));
        std::string rhs = trim(s.substr(eq+1));
        if (!lhs.empty()) {
            Value v = eval_expr(rhs, env);
            env.vars[lhs] = v;
            return;
        }
    }

    Value v = eval_expr(s, env);
    std::cout << v.repr() << std::endl;
}

static std::string read_file(const std::string & path) {
    std::ifstream ifs(path);
    if (!ifs) die("cannot open file: " + path);
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static void cmd_run(const std::string & path) {
    std::string code = read_file(path);
    Env env;
    std::istringstream iss(code);
    std::string line;
    while (std::getline(iss, line)) {
        exec_line(line, env);
    }
}

static void cmd_repl_flow() {
    std::cout << "FlowLang REPL (Python-like interpreter)\n";
    std::cout << "Supported: integers, strings, + - * /, assignment, print()\n";
    std::cout << "Type 'exit' to quit.\n\n";

    Env env;
    std::string line;
    while (true) {
        std::cout << "flow> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit") break;
        exec_line(line, env);
    }
}

// ======================================================
// FlowPM + .flowpkg
// ======================================================

static const std::string BASE_URL = "https://owenjz20.github.io";

static size_t write_to_string(void * ptr, size_t size, size_t nmemb, void * userdata) {
    size_t total = size * nmemb;
    std::string * s = static_cast<std::string *>(userdata);
    s->append(static_cast<char *>(ptr), total);
    return total;
}

static void curl_init_once() {
    static bool inited = false;
    if (!inited) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        inited = true;
    }
}

static std::string http_get(const std::string & url) {
    curl_init_once();
    CURL * curl = curl_easy_init();
    if (!curl) die("curl_easy_init failed");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) die("HTTP GET failed");

    return response;
}

static void http_download_to_file(const std::string & url, const std::string & path) {
    curl_init_once();
    CURL * curl = curl_easy_init();
    if (!curl) die("curl_easy_init failed");

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) die("failed to open file for writing");

    auto write_cb = [](void * ptr, size_t size, size_t nmemb, void * userdata) -> size_t {
        std::ofstream * ofs = static_cast<std::ofstream *>(userdata);
        size_t total = size * nmemb;
        ofs->write((char *)ptr, total);
        return total;
    };

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}

static void cmd_pm_list() {
    std::string url = BASE_URL + "/packages.json";
    std::cout << http_get(url) << std::endl;
}

static void cmd_pm_download(const std::string & name) {
    std::string url = BASE_URL + "/packages/" + name + ".flpkg";
    http_download_to_file(url, name + ".flpkg");
    std::cout << "Downloaded " << name << std::endl;
}

static void cmd_pkg_make(const std::string & folder) {
    if (!fs::exists(folder)) die("folder does not exist");

    std::string pkg_dir = folder + ".flowpkg";
    if (fs::exists(pkg_dir)) fs::remove_all(pkg_dir);
    fs::create_directories(pkg_dir);

    for (auto & p : fs::recursive_directory_iterator(folder)) {
        if (fs::is_directory(p)) continue;

        auto rel = fs::relative(p.path(), folder);
        fs::path dest = fs::path(pkg_dir) / rel;

        fs::create_directories(dest.parent_path());
        fs::copy_file(p.path(), dest, fs::copy_options::overwrite_existing);
    }

    std::cout << "Created package: " << pkg_dir << "\n";
}

// ======================================================
// Help + main
// ======================================================

static void print_help() {
    std::cout <<
        "FlowLang CLI\n"
        "Usage:\n"
        "  flowlang llm \"prompt\" [max_tokens]\n"
        "  flowlang ide [model]\n"
        "  flowlang repl-llm [model]\n"
        "  flowlang repl-flow           # Python-like interpreter REPL\n"
        "  flowlang run <file.flow>     # run FlowLang script\n"
        "  flowlang pm list\n"
        "  flowlang pm download <name>\n"
        "  flowlang pkg make <folder>\n"
        "  flowlang help\n";
}

int main(int argc, char ** argv) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "llm") {
        if (argc < 3) die("llm requires a prompt");
        std::string prompt = argv[2];
        int max_tokens = (argc >= 4) ? std::stoi(argv[3]) : 128;
        cmd_llm(prompt, "models/model.gguf", max_tokens);
    }
    else if (cmd == "ide") {
        std::string model = (argc >= 3) ? argv[2] : "models/model.gguf";
        cmd_ide(model);
    }
    else if (cmd == "repl-llm") {
        std::string model = (argc >= 3) ? argv[2] : "models/model.gguf";
        cmd_repl_llm(model);
    }
    else if (cmd == "repl-flow") {
        cmd_repl_flow();
    }
    else if (cmd == "run") {
        if (argc < 3) die("run requires file");
        cmd_run(argv[2]);
    }
    else if (cmd == "pm") {
        if (argc < 3) die("pm requires subcommand");
        std::string sub = argv[2];
        if (sub == "list") cmd_pm_list();
        else if (sub == "download") {
            if (argc < 4) die("pm download requires name");
            cmd_pm_download(argv[3]);
        }
        else die("unknown pm subcommand");
    }
    else if (cmd == "pkg") {
        if (argc < 3) die("pkg requires subcommand");
        if (std::string(argv[2]) == "make") {
            if (argc < 4) die("pkg make requires folder");
            cmd_pkg_make(argv[3]);
        } else die("unknown pkg subcommand");
    }
    else {
        print_help();
    }

    return 0;
}
