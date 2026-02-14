using System;
using System.IO;

namespace FlowLangAI
{
    internal class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("Usage: FlowLangAI.exe <file.lang>");
                return;
            }

            string langPath = args[0];
            if (!File.Exists(langPath))
            {
                Console.WriteLine("File not found: " + langPath);
                return;
            }

            string prompt = File.ReadAllText(langPath);

            var engine = new FlowLangAIEngine("model.gguf");
            string flowCode = engine.GenerateFlowLang(prompt);

            string outPath = Path.ChangeExtension(langPath, ".flow");
            File.WriteAllText(outPath, flowCode);

            Console.WriteLine("Generated: " + outPath);
        }
    }
}
