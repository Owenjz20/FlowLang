using System;
using System.Runtime.InteropServices;

namespace FlowLangAI
{
    public static class LlamaNative
    {
        [DllImport("llama.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr llama_load_model_from_file(
            [MarshalAs(UnmanagedType.LPStr)] string path,
            IntPtr paramsPtr);

        [DllImport("llama.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr llama_new_context_with_model(
            IntPtr model,
            IntPtr contextParams);

        [DllImport("llama.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int llama_tokenize(
            IntPtr ctx,
            [MarshalAs(UnmanagedType.LPStr)] string text,
            int[] tokens,
            int maxTokens,
            bool addBos);

        [DllImport("llama.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int llama_eval(
            IntPtr ctx,
            int[] tokens,
            int nTokens,
            int nPast);

        [DllImport("llama.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int llama_sample_token(IntPtr ctx);
    }
}
