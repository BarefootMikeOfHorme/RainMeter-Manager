using Microsoft.Extensions.Logging;
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;

namespace RenderProcess.Content;

public enum WebSourceType { Page, Video, Image, JSON }

public record CuratedSource(string Id, string Name, string Url, WebSourceType Type, string Attribution = "");

public class WebContentLoader
{
    private readonly ILogger<WebContentLoader> _logger;
    private readonly HttpClient _http;

    public WebContentLoader(ILogger<WebContentLoader> logger)
    {
        _logger = logger;
        _http = new HttpClient();
        _http.Timeout = TimeSpan.FromSeconds(10);
    }

    public IReadOnlyList<CuratedSource> GetCuratedSources() => new List<CuratedSource>
    {
        // NASA APOD (uses DEMO_KEY by default; set NASA_API_KEY env var to override)
        new("nasa_apod", "NASA Astronomy Picture of the Day", "https://api.nasa.gov/planetary/apod?api_key=DEMO_KEY", WebSourceType.JSON, "NASA APOD (Public Domain, attribution appreciated)"),

        // NASA Image and Video Library sample search (JSON)
        new("nasa_images_moon", "NASA Images - Moon", "https://images-api.nasa.gov/search?q=moon&media_type=image", WebSourceType.JSON, "NASA Image & Video Library (Public Domain)"),

        // NASA Mars Rover photos (latest Perseverance)
        new("nasa_mars_rover", "NASA Mars Rover (Perseverance)", "https://api.nasa.gov/mars-photos/api/v1/rovers/perseverance/latest_photos?api_key=DEMO_KEY", WebSourceType.JSON, "NASA Mars Rover Photos (Public Domain)"),

        // NOAA Weather Radar (animated GIF)
        new("noaa_conus_radar", "NOAA CONUS Radar (Animated)", "https://radar.weather.gov/ridge/Conus/RadarImg/latest_radaronly.gif", WebSourceType.Image, "NOAA/NWS Weather Radar (US Public Domain)"),

        // Weather.gov example API (JSON)
        new("noaa_points_dc", "NOAA Weather Point (DC)", "https://api.weather.gov/points/38.8894,-77.0352", WebSourceType.JSON, "NOAA/NWS API (US Public Domain)"),

        // USGS Earthquakes (Past Day)
        new("usgs_eq_day", "USGS Earthquakes (Past Day)", "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_day.geojson", WebSourceType.JSON, "USGS Earthquake Hazards Program (Public Domain)"),

        // Open-Meteo sample forecast (no API key)
        new("open_meteo_dc", "Open-Meteo Forecast (DC)", "https://api.open-meteo.com/v1/forecast?latitude=38.8894&longitude=-77.0352&hourly=temperature_2m,relative_humidity_2m,precipitation", WebSourceType.JSON, "Open-Meteo (CC BY 4.0)"),

        // Wikimedia / Wikipedia safe media
        new("wikimedia_featured_sample", "Wikimedia Sample Image", "https://upload.wikimedia.org/wikipedia/commons/1/15/Red_Kitten_01.jpg", WebSourceType.Image, "Wikimedia Commons (license per file)"),
        new("wikipedia_featured", "Wikipedia Featured Feed (EN, sample date)", "https://en.wikipedia.org/api/rest_v1/feed/featured/2024/01/01", WebSourceType.JSON, "Wikipedia REST API"),

        // OpenStreetMap static tile
        new("osm_world_tile", "OpenStreetMap World Tile", "https://tile.openstreetmap.org/2/1/1.png", WebSourceType.Image, "© OpenStreetMap contributors"),

        // YouTube privacy-enhanced embedding (NoCookie)
        new("youtube_nocookie_sample", "YouTube (NoCookie) Sample", "https://www.youtube-nocookie.com/watch?v=dQw4w9WgXcQ", WebSourceType.Video, "YouTube (privacy-enhanced)")
    };

public async Task<(bool ok, string html)> BuildEmbedHtmlAsync(CuratedSource source)
    {
        try
        {
            // Special handling for YouTube
            if (IsYouTubeUrl(source.Url))
            {
                var vid = ExtractYouTubeId(source.Url);
                if (!string.IsNullOrWhiteSpace(vid))
                {
                    var embed = $"<iframe src='https://www.youtube.com/embed/{vid}?autoplay=1&mute=1&loop=1&playlist={vid}' style='width:100%;height:100%;border:0' allow='autoplay; encrypted-media' sandbox='allow-scripts allow-same-origin' referrerpolicy='no-referrer' loading='lazy'></iframe>";
                    return (true, embed);
                }
            }

            switch (source.Type)
            {
                case WebSourceType.Image:
                    return (true, $"<img alt='{Escape(source.Name)}' src='{source.Url}' style='max-width:100%;height:auto' />");

                case WebSourceType.Page:
                    return (true, $"<iframe src='{source.Url}' style='width:100%;height:100%;border:0' sandbox='allow-scripts allow-same-origin' referrerpolicy='no-referrer' loading='lazy'></iframe>");

                case WebSourceType.Video:
                    // For direct MP4/HLS links use <video> tag; for YouTube-like, rely on iframe embedding
                    return (true, $"<video src='{source.Url}' autoplay muted loop controls style='width:100%;height:auto'></video>");

                case WebSourceType.JSON:
                    var (ok, jsonHtml) = await FetchAndFormatJsonAsync(source.Url);
                    return (ok, jsonHtml);

                default:
                    return (false, "Unsupported source type");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to build embed html for {id}", source.Id);
            return (false, ex.Message);
        }
    }

    private static bool IsYouTubeUrl(string url)
    {
        return url.Contains("youtube.com", StringComparison.OrdinalIgnoreCase) ||
               url.Contains("youtu.be", StringComparison.OrdinalIgnoreCase) ||
               url.Contains("youtube-nocookie.com", StringComparison.OrdinalIgnoreCase);
    }

    private static string ExtractYouTubeId(string url)
    {
        try
        {
            // Handle watch?v=ID, youtu.be/ID, embed/ID
            var uri = new Uri(url);
            if (uri.Host.Contains("youtu.be", StringComparison.OrdinalIgnoreCase))
            {
                return uri.AbsolutePath.Trim('/');
            }
            var v = GetQueryParam(uri, "v");
            if (!string.IsNullOrWhiteSpace(v)) return v;
            var segs = uri.Segments;
            if (segs.Length > 1 && segs[segs.Length - 2].Trim('/').Equals("embed", StringComparison.OrdinalIgnoreCase))
            {
                return segs[^1].Trim('/');
            }
        }
        catch { }
        return string.Empty;
    }

    private static string GetQueryParam(Uri uri, string key)
    {
        try
        {
            var query = uri.Query;
            if (string.IsNullOrEmpty(query)) return string.Empty;
            if (query.StartsWith("?")) query = query.Substring(1);
            var parts = query.Split('&', StringSplitOptions.RemoveEmptyEntries);
            foreach (var part in parts)
            {
                var kv = part.Split('=', 2);
                if (kv.Length >= 1 && kv[0].Equals(key, StringComparison.OrdinalIgnoreCase))
                {
                    return kv.Length == 2 ? Uri.UnescapeDataString(kv[1]) : string.Empty;
                }
            }
        }
        catch { }
        return string.Empty;
    }

    private async Task<(bool ok, string html)> FetchAndFormatJsonAsync(string url)
    {
        try
        {
            // Substitute NASA DEMO_KEY with user key if available
            if (url.Contains("api.nasa.gov") && url.Contains("DEMO_KEY"))
            {
                var key = Environment.GetEnvironmentVariable("NASA_API_KEY");
                if (!string.IsNullOrWhiteSpace(key))
                {
                    url = url.Replace("DEMO_KEY", key);
                }
            }

            var json = await _http.GetStringAsync(url);
            var pretty = JsonSerializer.Serialize(JsonSerializer.Deserialize<object>(json), new JsonSerializerOptions { WriteIndented = true });
            var html = $@"<div style='font-family:Consolas,monospace;white-space:pre-wrap;background:#1e1e1e;color:#dcdcdc;padding:10px;border-radius:8px;'>
<h3 style='margin:0 0 10px 0;font-family:Segoe UI,Arial,sans-serif;'>JSON Response</h3>
<pre>{Escape(pretty)}</pre>
</div>";
            return (true, html);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to fetch JSON from {url}", url);
            var err = $"<div style='color:#ff6666;font-family:Segoe UI,Arial,sans-serif;'>Error fetching JSON: {Escape(ex.Message)}</div>";
            return (false, err);
        }
    }

    private static string Escape(string s) => System.Net.WebUtility.HtmlEncode(s ?? string.Empty);
}

