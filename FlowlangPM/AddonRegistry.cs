using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

namespace FlowLangPM
{
    public class AddonRegistry
    {
        public class Entry
        {
            public string Name { get; set; } = "";
            public string Version { get; set; } = "";
            public string File { get; set; } = "";
        }

        public List<Entry> Installed { get; set; } = new();

        private static string RegistryPath =>
            Path.Combine(AppContext.BaseDirectory, "addons", "registry.json");

        public static AddonRegistry Load()
        {
            if (!File.Exists(RegistryPath))
                return new AddonRegistry();

            string json = File.ReadAllText(RegistryPath);
            return JsonSerializer.Deserialize<AddonRegistry>(json) ?? new AddonRegistry();
        }

        public void Save()
        {
            Directory.CreateDirectory(Path.Combine(AppContext.BaseDirectory, "addons"));
            string json = JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(RegistryPath, json);
        }

        public void Add(AddonMetadata meta, string fileName)
        {
            Installed.RemoveAll(x => x.Name == meta.Name);
            Installed.Add(new Entry
            {
                Name = meta.Name,
                Version = meta.Version,
                File = fileName
            });
        }

        public void Remove(string name)
        {
            Installed.RemoveAll(x => x.Name == name);
        }

        public Entry? Find(string name)
        {
            return Installed.Find(x => x.Name == name);
        }
    }
}
