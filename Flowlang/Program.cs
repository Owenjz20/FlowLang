using System;
using System.IO;

namespace FlowLangRuntime
{
    internal class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.WriteLine("Usage: FlowLang.exe <file.flow>");
                return;
            }

            string path = args[0];
            if (!File.Exists(path))
            {
                Console.WriteLine("File not found: " + path);
                return;
            }

            string code = File.ReadAllText(path);
            var interpreter = new FlowInterpreter(Console.WriteLine);
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
