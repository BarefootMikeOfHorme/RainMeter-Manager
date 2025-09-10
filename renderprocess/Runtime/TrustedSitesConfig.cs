using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;

namespace RenderProcess.Runtime;

public enum SiteTrustPolicy
{
    Block,
    External,
    AllowOnce,
    AlwaysAllow,
    Restricted
}

public sealed class TrustedSitesConfig
{
    public Dictionary<string, SiteTrustPolicy> Sites { get; set; } = new(StringComparer.OrdinalIgnoreCase);

    public static TrustedSitesConfig Load(string path)
    {
        try
        {
            if (File.Exists(path))
            {
                var json = File.ReadAllText(path);
                var cfg = JsonSerializer.Deserialize<TrustedSitesConfig>(json, new JsonSerializerOptions { ReadCommentHandling = JsonCommentHandling.Skip, AllowTrailingCommas = true });
                return cfg ?? new TrustedSitesConfig();
            }
        }
        catch { }
        return new TrustedSitesConfig();
    }

    public void Save(string path)
    {
        try
        {
            var dir = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir)) Directory.CreateDirectory(dir);
            var json = JsonSerializer.Serialize(this, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(path, json);
        }
        catch { }
    }
}

