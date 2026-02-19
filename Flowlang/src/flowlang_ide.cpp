#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <filesystem>

extern "C" {
#include "llama.h"
}

static std::string build_prompt(const std::string &spec) {
    std::ostringstream ss;
    ss << "<s>[INSTRUCTION]\n";
    ss << "You are FlowLang, a code generator for the FlowLang language.\n";
    ss << "Given a natural language description, output ONLY valid FlowLang code.\n\n";
    ss << "[INSTRUCTION]\n" << spec << "\n\n";
    ss << "[OUTPUT]\n";
    return ss.str();
}

static std::string generate(const std::string &model_path,
                            const std::string &prompt,
                            int max_tokens = 512) {

    llama_backend_init();

    // model params
    llama_model_params mparams = llama_model_default_params();
    llama_model *model = llama_load_model_from_file(model_path.c_str(), &mparams);
    if (!model) throw std::runtime_error("Failed to load model");

    // context params
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx    = 2048;
    cparams.n_threads = 4;

    llama_context *ctx = llama_new_context_with_model(model, &cparams);
    if (!ctx) throw std::runtime_error("Failed to create context");

    // tokenize
    std::vector<int32_t> tokens(4096);
    int n = llama_tokenize(model, prompt.c_str(), (int)prompt.size(),
                           tokens.data(), (int)tokens.size(), true);
    if (n < 0) throw std::runtime_error("Tokenization failed");
    tokens.resize(n);

    // build batch for prompt
    llama_batch batch = llama_batch_init(512, 0, 1);
    for (int i = 0; i < n; ++i) {
        batch.token[i]      = tokens[i];
        batch.pos[i]        = i;
        batch.n_seq_id[i]   = 1;
        batch.seq_id[i][0]  = 0;
    }
    batch.n_tokens = n;

    if (llama_decode(ctx, batch) != 0)
        throw std::runtime_error("Initial decode failed");

    std::string out;
    out.reserve(max_tokens * 4);

    for (int i = 0; i < max_tokens; ++i) {
        int32_t tok = llama_sample_token_greedy(ctx);
        if (tok == llama_token_eos(model)) break;

        char buf[256];
        int len = llama_token_to_piece(model, tok, buf, (int)sizeof(buf), true);
        if (len > 0) out.append(buf, len);

        // feed token back
        batch.token[0]     = tok;
        batch.pos[0]       = n + i;
        batch.n_tokens     = 1;
        batch.n_seq_id[0]  = 1;
        batch.seq_id[0][0] = 0;

        if (llama_decode(ctx, batch) != 0) break;
    }

    llama_batch_free(batch);
    llama_free(ctx);
    llama_free_model(model);
    llama_backend_free();

    return out;
}

int main(int argc, char **argv) {
    std::string model = (argc >= 2) ? argv[1] : "models/flowlang/model.gguf";

    std::cout << "FlowLang IDE\n";
    std::cout << "Model: " << model << "\n";
    std::cout << "Enter description, then press ENTER twice.\n\n";

    while (true) {
        std::cout << ">>> ";
        std::string line, spec;
        while (std::getline(std::cin, line)) {
            if (line.empty()) break;
            spec += line + "\n";
        }
        if (spec.empty()) break;

        std::string prompt = build_prompt(spec);
        std::string code   = generate(model, prompt);

        std::cout << "\n[FlowLang Output]\n" << code << "\n\n";
    }

    return 0;
}
