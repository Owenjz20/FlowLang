using System;
using System.IO;

namespace FlowLangPM
{
    public class AddonMetadata
    {
        public string Name { get; set; } = "";
        public string Version { get; set; } = "";
        public string Author { get; set; } = "";
        public string Description { get; set; } = "";

        public static AddonMetadata Parse(string path)
        {
            var meta = new AddonMetadata();
            var lines = File.ReadAllLines(path);

            foreach (var line in lines)
            {
                var t = line.Trim();

                if (t.StartsWith("addon "))
                    meta.Name = ExtractString(t);
                else if (t.StartsWith("version "))
                    meta.Version = ExtractString(t);
                else if (t.StartsWith("author "))
                    meta.Author = ExtractString(t);
                else if (t.StartsWith("description "))
                    meta.Description = ExtractString(t);
            }

            return meta;
        }

        private static string ExtractString(string line)
        {
            int start = line.IndexOf('"') + 1;
            int end = line.LastIndexOf('"');
            return line.Substring(start, end - start);
        }
    }
}
