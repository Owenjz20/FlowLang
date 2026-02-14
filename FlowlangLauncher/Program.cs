using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace FlowLangLauncher
{
    internal class Program
    {
        static void Main(string[] args)
        {
            string baseDir = AppContext.BaseDirectory;
            string localVersionFile = Path.Combine(baseDir, "version.txt");
            string updateDir = Path.Combine(baseDir, "updates"); // pretend this is "server"

            string localVersion = ReadVersion(localVersionFile);
            string latestVersion = ReadVersion(Path.Combine(updateDir, "version.txt"));

            if (!string.IsNullOrEmpty(latestVersion) &&
                IsNewer(latestVersion, localVersion))
            {
                Console.WriteLine($"Updating FlowLang {localVersion} -> {latestVersion}...");
                ApplyUpdate(updateDir, baseDir);
                File.WriteAllText(localVersionFile, latestVersion);
            }

            // Launch IDE by default
            string idePath = Path.Combine(baseDir, "FlowLangIDE.exe");
            if (File.Exists(idePath))
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = idePath,
                    UseShellExecute = true
                });
            }
            else
            {
                Console.WriteLine("FlowLangIDE.exe not found.");
            }
        }

        static string ReadVersion(string path)
        {
            if (!File.Exists(path)) return "0.0.0";
            return File.ReadAllText(path).Trim();
        }

        static bool IsNewer(string a, string b)
        {
            Version va, vb;
            if (!Version.TryParse(a, out va)) va = new Version(0, 0, 0);
            if (!Version.TryParse(b, out vb)) vb = new Version(0, 0, 0);
            return va > vb;
        }

        static void ApplyUpdate(string updateDir, string baseDir)
        {
            if (!Directory.Exists(updateDir)) return;

            foreach (var file in Directory.GetFiles(updateDir))
            {
                string name = Path.GetFileName(file);
                if (name.Equals("version.txt", StringComparison.OrdinalIgnoreCase))
                    continue;

                string dest = Path.Combine(baseDir, name);
                File.Copy(file, dest, true);
            }
        }
    }
}
