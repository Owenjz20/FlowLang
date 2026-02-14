using System;
using System.IO;
using System.Text;

namespace FlowLangAI
{
    public class FlowLangAIEngine
    {
        private IntPtr _model;
        private IntPtr _ctx;
        private string _systemPrompt;

        public FlowLangAIEngine(string modelPath)
        {
            _systemPrompt = File.ReadAllText("prompt.txt");

            _model = LlamaNative.llama_load_model_from_file(modelPath, IntPtr.Zero);
            if (_model == IntPtr.Zero)
                throw new Exception("Failed to load model");

            _ctx = LlamaNative.llama_new_context_with_model(_model, IntPtr.Zero);
            if (_ctx == IntPtr.Zero)
                throw new Exception("Failed to create context");
        }

        public string GenerateFlowLang(string userPrompt)
        {
            string fullPrompt =
                _systemPrompt +
                "\n\nUser request:\n" +
                userPrompt +
                "\n\nFlowLang code:\n";

            return RunModel(fullPrompt);
        }

        private string RunModel(string prompt)
        {
            int[] tokens = new int[4096];
            int count = LlamaNative.llama_tokenize(_ctx, prompt, tokens, tokens.Length, true);

            LlamaNative.llama_eval(_ctx, tokens, count, 0);

            var sb = new StringBuilder();

            for (int i = 0; i < 2048; i++)
            {
                int token = LlamaNative.llama_sample_token(_ctx);
                if (token < 0)
                    break;

                sb.Append(TokenToString(token));
            }

            return sb.ToString();
        }

        private string TokenToString(int token)
        {
            // TODO: real vocab decoding
            return "[" + token + "]";
        }
    }
}
