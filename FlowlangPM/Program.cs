using System;
using System.IO;

namespace FlowLangPM
{
    internal class Program
    {
        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                PrintHelp();
                return;
            }

            string cmd = args[0].ToLower();
            var registry = AddonRegistry.Load();

            switch (cmd)
            {
                case "install":
                    if (args.Length < 2)
                    {
                        Console.WriteLine("Usage: FlowLangPM install <file.flowaddon>");
                        return;
                    }
                    InstallAddon(args[1], registry);
                    break;

                case "uninstall":
                    if (args.Length < 2)
                    {
                        Console.WriteLine("Usage: FlowLangPM uninstall <addonName>");
                        return;
                    }
                    UninstallAddon(args[1], registry);
                    break;

                case "list":
                    ListAddons(registry);
                    break;

                case "info":
                    if (args.Length < 2)
                    {
                        Console.WriteLine("Usage: FlowLangPM info <addonName>");
                        return;
                    }
                    ShowInfo(args[1], registry);
                    break;

                case "update":
                    if (args.Length < 2)
                    {
                        Console.WriteLine("Usage: FlowLangPM update <file.flowaddon>");
                        return;
                    }
                    InstallAddon(args[1], registry);
                    break;

                default:
                    PrintHelp();
                    break;
            }
        }

        static void PrintHelp()
        {
            Console.WriteLine("FlowLang Package Manager");
            Console.WriteLine("Commands:");
            Console.WriteLine("  install <file.flowaddon>");
            Console.WriteLine("  uninstall <addonName>");
            Console.WriteLine("  list");
            Console.WriteLine("  info <addonName>");
            Console.WriteLine("  update <file.flowaddon>");
        }

        static void InstallAddon(string path, AddonRegistry registry)
        {
            if (!File.Exists(path))
            {
                Console.WriteLine("File not found: " + path);
                return;
            }

            var meta = AddonMetadata.Parse(path);
            string addonsDir = Path.Combine(AppContext.BaseDirectory, "addons");
            Directory.CreateDirectory(addonsDir);

            string dest = Path.Combine(addonsDir, meta.Name + ".flowaddon");
            File.Copy(path, dest, true);

            registry.Add(meta, meta.Name + ".flowaddon");
            registry.Save();

            Console.WriteLine($"Installed addon: {meta.Name} {meta.Version}");
        }

        static void UninstallAddon(string name, AddonRegistry registry)
        {
            var entry = registry.Find(name);
            if (entry == null)
            {
                Console.WriteLine("Addon not installed: " + name);
                return;
            }

            string file = Path.Combine(AppContext.BaseDirectory, "addons", entry.File);
            if (File.Exists(file))
                File.Delete(file);

            registry.Remove(name);
            registry.Save();

            Console.WriteLine("Uninstalled addon: " + name);
        }

        static void ListAddons(AddonRegistry registry)
        {
            Console.WriteLine("Installed addons:");
            foreach (var a in registry.Installed)
                Console.WriteLine($"  {a.Name} {a.Version}");
        }

        static void ShowInfo(string name, AddonRegistry registry)
        {
            var entry = registry.Find(name);
            if (entry == null)
            {
                Console.WriteLine("Addon not installed: " + name);
                return;
            }

            string path = Path.Combine(AppContext.BaseDirectory, "addons", entry.File);
            var meta = AddonMetadata.Parse(path);

            Console.WriteLine($"Name: {meta.Name}");
            Console.WriteLine($"Version: {meta.Version}");
            Console.WriteLine($"Author: {meta.Author}");
            Console.WriteLine($"Description: {meta.Description}");
        }
    }
}
