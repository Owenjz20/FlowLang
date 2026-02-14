using System;
using System.IO;

namespace FlowLangRuntime
{
    internal class Program
    {
        static void Main(string[] args)
        {
            // No file provided
            if (args.Length == 0)
            {
                Console.WriteLine("FlowLang Runtime");
                Console.WriteLine("Usage: FlowLang.exe <file.flow>");
                return;
            }

            string path = args[0];

            // File missing
            if (!File.Exists(path))
            {
                Console.WriteLine("File not found: " + path);
                return;
            }

            // Load FlowLang code
            string code = File.ReadAllText(path);

            // Create interpreter
            var interpreter = new FlowInterpreter(Console.WriteLine);

            // Run program
            try
            {
                interpreter.Run(code);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Runtime error: " + ex.Message);
            }
        }
    }
}
