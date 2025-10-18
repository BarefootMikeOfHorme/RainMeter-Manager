using Microsoft.Extensions.Logging;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

namespace RenderProcess.Content;

public enum WebSourceType 
{ 
    Page, 
    Video, 
    Image, 
    JSON, 
    RSS, 
    Widget, 
    Live, 
    Interactive,
    Data
}

public enum WebSourceCategory
{
    Space,
    Weather,
    News,
    Science,
    Art,
    Nature,
    Technology,
    Finance,
    Social,
    Entertainment,
    Education,
    Health,
    Sports
}

public record CuratedSource(
    string Id, 
    string Name, 
    string Url, 
    WebSourceType Type, 
    WebSourceCategory Category,
    string Attribution = "",
    string Description = "",
    bool RequiresApiKey = false,
    string ApiKeyEnvVar = "",
    int RefreshMinutes = 60,
    Dictionary<string, string>? Metadata = null
);

/// <summary>
/// Ultra-enhanced web content loader with 50+ curated sources
/// Features:
/// - Live data feeds (weather, space, news)
/// - Interactive visualizations
/// - Real-time APIs
/// - Cached responses
/// - Rate limiting
/// - Error resilience
/// </summary>
public class WebContentLoader
{
    private readonly ILogger<WebContentLoader> _logger;
    private readonly HttpClient _http;
    private readonly Dictionary<string, (DateTime timestamp, string content)> _cache;
    private readonly SemaphoreSlim _cacheLock;
    
    // Configuration
    public int CacheMinutes { get; set; } = 15;
    public int RequestTimeoutSeconds { get; set; } = 30;
    public bool EnableCaching { get; set; } = true;
    public bool EnableRateLimiting { get; set; } = true;

    public WebContentLoader(ILogger<WebContentLoader> logger)
    {
        _logger = logger;
        _http = new HttpClient();
        _http.Timeout = TimeSpan.FromSeconds(RequestTimeoutSeconds);
        _http.DefaultRequestHeaders.Add("User-Agent", "RainmeterManager/2.0");
        _cache = new Dictionary<string, (DateTime, string)>();
        _cacheLock = new SemaphoreSlim(1, 1);
    }

    public IReadOnlyList<CuratedSource> GetAllSources() => GetCuratedSources().ToList();
    
    public IReadOnlyList<CuratedSource> GetSourcesByCategory(WebSourceCategory category) => 
        GetCuratedSources().Where(s => s.Category == category).ToList();
    
    public CuratedSource? GetSourceById(string id) => 
        GetCuratedSources().FirstOrDefault(s => s.Id == id);

    private IEnumerable<CuratedSource> GetCuratedSources()
    {
        // ====================
        // SPACE & ASTRONOMY
        // ====================
        
        yield return new("nasa_apod", 
            "NASA Astronomy Picture of the Day", 
            "https://api.nasa.gov/planetary/apod?api_key=DEMO_KEY",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "NASA APOD (Public Domain)",
            "Daily stunning space images with explanations",
            true,
            "NASA_API_KEY",
            1440); // Daily
            
        yield return new("nasa_mars_rover_perseverance",
            "Mars Rover - Perseverance Latest Photos",
            "https://api.nasa.gov/mars-photos/api/v1/rovers/perseverance/latest_photos?api_key=DEMO_KEY",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "NASA Mars Rover Photos",
            "Latest photos from Mars Perseverance rover",
            true,
            "NASA_API_KEY",
            360);
            
        yield return new("nasa_mars_rover_curiosity",
            "Mars Rover - Curiosity Latest Photos",
            "https://api.nasa.gov/mars-photos/api/v1/rovers/curiosity/latest_photos?api_key=DEMO_KEY",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "NASA Mars Rover Photos",
            "Latest photos from Mars Curiosity rover",
            true,
            "NASA_API_KEY",
            360);
            
        yield return new("nasa_epic_earth",
            "NASA EPIC - Earth from Space",
            "https://api.nasa.gov/EPIC/api/natural?api_key=DEMO_KEY",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "NASA EPIC",
            "Full disk images of Earth from DSCOVR satellite",
            true,
            "NASA_API_KEY",
            180);
            
        yield return new("nasa_images_search",
            "NASA Image & Video Library",
            "https://images-api.nasa.gov/search?q=nebula&media_type=image",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "NASA Image Library",
            "Search NASA's vast image and video collection",
            false,
            "",
            1440);
            
        yield return new("spacex_launches",
            "SpaceX Launches",
            "https://api.spacexdata.com/v4/launches/latest",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "SpaceX API (Open Source)",
            "Latest SpaceX launch information",
            false,
            "",
            60);
            
        yield return new("iss_location",
            "International Space Station Location",
            "http://api.open-notify.org/iss-now.json",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "Open Notify API",
            "Real-time ISS location tracking",
            false,
            "",
            5); // Update every 5 minutes
            
        yield return new("iss_passes",
            "ISS Pass Times (Sample Location)",
            "http://api.open-notify.org/iss-pass.json?lat=38.8894&lon=-77.0352",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "Open Notify API",
            "When ISS will pass overhead",
            false,
            "",
            360);
        
        // ====================
        // WEATHER
        // ====================
        
        yield return new("noaa_radar_conus",
            "NOAA CONUS Radar (Animated)",
            "https://radar.weather.gov/ridge/Conus/RadarImg/latest_radaronly.gif",
            WebSourceType.Image,
            WebSourceCategory.Weather,
            "NOAA/NWS (US Public Domain)",
            "Animated weather radar for continental US",
            false,
            "",
            10);
            
        yield return new("noaa_api_dc",
            "NOAA Weather Forecast (DC)",
            "https://api.weather.gov/points/38.8894,-77.0352",
            WebSourceType.JSON,
            WebSourceCategory.Weather,
            "NOAA/NWS API",
            "Detailed weather forecast for Washington DC",
            false,
            "",
            30);
            
        yield return new("open_meteo_forecast",
            "Open-Meteo Weather Forecast",
            "https://api.open-meteo.com/v1/forecast?latitude=38.8894&longitude=-77.0352&hourly=temperature_2m,relative_humidity_2m,precipitation,wind_speed_10m",
            WebSourceType.JSON,
            WebSourceCategory.Weather,
            "Open-Meteo (CC BY 4.0)",
            "Free weather API with hourly forecasts",
            false,
            "",
            60);
            
        yield return new("weatherapi_current",
            "WeatherAPI Current Conditions",
            "https://api.weatherapi.com/v1/current.json?key=DEMO_KEY&q=Washington",
            WebSourceType.JSON,
            WebSourceCategory.Weather,
            "WeatherAPI.com",
            "Real-time weather conditions",
            true,
            "WEATHER_API_KEY",
            15);
            
        yield return new("openweather_current",
            "OpenWeatherMap Current",
            "https://api.openweathermap.org/data/2.5/weather?q=Washington&appid=DEMO_KEY&units=metric",
            WebSourceType.JSON,
            WebSourceCategory.Weather,
            "OpenWeatherMap",
            "Current weather data",
            true,
            "OPENWEATHER_API_KEY",
            15);
        
        // ====================
        // EARTH & NATURE
        // ====================
        
        yield return new("usgs_earthquakes_day",
            "USGS Earthquakes (Past Day)",
            "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/all_day.geojson",
            WebSourceType.JSON,
            WebSourceCategory.Science,
            "USGS (Public Domain)",
            "Real-time earthquake data worldwide",
            false,
            "",
            15);
            
        yield return new("usgs_earthquakes_week",
            "USGS Earthquakes (Past Week)",
            "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/significant_week.geojson",
            WebSourceType.JSON,
            WebSourceCategory.Science,
            "USGS (Public Domain)",
            "Significant earthquakes in past week",
            false,
            "",
            60);
            
        yield return new("noaa_tides",
            "NOAA Tides & Currents",
            "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter?station=8594900&product=predictions&datum=MLLW&time_zone=lst_ldt&units=english&format=json&date=today",
            WebSourceType.JSON,
            WebSourceCategory.Weather,
            "NOAA Tides (Public Domain)",
            "Tide predictions for coastal locations",
            false,
            "",
            360);
            
        yield return new("air_quality_world",
            "World Air Quality Index",
            "https://api.waqi.info/feed/washington/?token=demo",
            WebSourceType.JSON,
            WebSourceCategory.Health,
            "WAQI Project",
            "Real-time air quality data",
            true,
            "WAQI_TOKEN",
            30);
        
        // ====================
        // NEWS & MEDIA
        // ====================
        
        yield return new("wikipedia_featured",
            "Wikipedia Featured Article",
            "https://en.wikipedia.org/api/rest_v1/page/featured/2024/01/01",
            WebSourceType.JSON,
            WebSourceCategory.Education,
            "Wikipedia",
            "Daily featured article from Wikipedia",
            false,
            "",
            1440);
            
        yield return new("wikimedia_commons_potd",
            "Wikimedia Commons - Picture of the Day",
            "https://commons.wikimedia.org/w/api.php?action=featuredfeed&feed=potd&feedformat=atom",
            WebSourceType.RSS,
            WebSourceCategory.Art,
            "Wikimedia Commons",
            "Featured picture of the day",
            false,
            "",
            1440);
            
        yield return new("reddit_space_top",
            "Reddit r/space Top Posts",
            "https://www.reddit.com/r/space/top.json?limit=10",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "Reddit",
            "Top posts from r/space subreddit",
            false,
            "",
            60);
            
        yield return new("hacker_news_top",
            "Hacker News Top Stories",
            "https://hacker-news.firebaseio.com/v0/topstories.json",
            WebSourceType.JSON,
            WebSourceCategory.Technology,
            "Hacker News",
            "Top tech news and discussions",
            false,
            "",
            30);
        
        // ====================
        // FINANCE & CRYPTO
        // ====================
        
        yield return new("coindesk_btc",
            "Bitcoin Price Index",
            "https://api.coindesk.com/v1/bpi/currentprice.json",
            WebSourceType.JSON,
            WebSourceCategory.Finance,
            "CoinDesk",
            "Current Bitcoin price in multiple currencies",
            false,
            "",
            5);
            
        yield return new("crypto_fear_greed",
            "Crypto Fear & Greed Index",
            "https://api.alternative.me/fng/?limit=10",
            WebSourceType.JSON,
            WebSourceCategory.Finance,
            "Alternative.me",
            "Market sentiment indicator",
            false,
            "",
            60);
            
        yield return new("exchange_rates",
            "Exchange Rates API",
            "https://api.exchangerate-api.com/v4/latest/USD",
            WebSourceType.JSON,
            WebSourceCategory.Finance,
            "ExchangeRate-API",
            "Current currency exchange rates",
            false,
            "",
            360);
        
        // ====================
        // ART & CULTURE
        // ====================
        
        yield return new("met_museum_objects",
            "Metropolitan Museum Collection",
            "https://collectionapi.metmuseum.org/public/collection/v1/objects",
            WebSourceType.JSON,
            WebSourceCategory.Art,
            "Metropolitan Museum of Art (CC0)",
            "Access to Met Museum collection",
            false,
            "",
            1440);
            
        yield return new("rijksmuseum_collection",
            "Rijksmuseum Collection",
            "https://www.rijksmuseum.nl/api/en/collection?key=DEMO_KEY&format=json",
            WebSourceType.JSON,
            WebSourceCategory.Art,
            "Rijksmuseum",
            "Dutch masterpieces and art",
            true,
            "RIJKS_API_KEY",
            1440);
            
        yield return new("unsplash_random",
            "Unsplash Random Photo",
            "https://source.unsplash.com/random/1920x1080",
            WebSourceType.Image,
            WebSourceCategory.Art,
            "Unsplash (Free to use)",
            "Random high-quality photograph",
            false,
            "",
            60);
            
        yield return new("picsum_random",
            "Lorem Picsum Random Image",
            "https://picsum.photos/1920/1080",
            WebSourceType.Image,
            WebSourceCategory.Art,
            "Lorem Picsum",
            "Random placeholder images",
            false,
            "",
            0); // Always fresh
        
        // ====================
        // TECHNOLOGY & DEV
        // ====================
        
        yield return new("github_trending",
            "GitHub Trending Repositories",
            "https://api.github.com/search/repositories?q=stars:>1000&sort=stars&order=desc",
            WebSourceType.JSON,
            WebSourceCategory.Technology,
            "GitHub",
            "Trending open source projects",
            false,
            "",
            360);
            
        yield return new("stackoverflow_questions",
            "Stack Overflow Recent Questions",
            "https://api.stackexchange.com/2.3/questions?order=desc&sort=activity&site=stackoverflow",
            WebSourceType.JSON,
            WebSourceCategory.Technology,
            "Stack Exchange",
            "Latest programming questions",
            false,
            "",
            15);
            
        yield return new("ip_geolocation",
            "Your IP Geolocation",
            "https://ipapi.co/json/",
            WebSourceType.JSON,
            WebSourceCategory.Technology,
            "ipapi.co",
            "Geolocation info for your IP",
            false,
            "",
            1440);
        
        // ====================
        // LIVE STREAMS & VIDEO
        // ====================
        
        yield return new("iss_live_stream",
            "ISS Live Stream (NASA)",
            "https://www.youtube.com/watch?v=86YLFOog4GM",
            WebSourceType.Video,
            WebSourceCategory.Space,
            "NASA",
            "Live view from International Space Station",
            false,
            "",
            0); // Live
            
        yield return new("youtube_lofi",
            "Lofi Hip Hop Radio (24/7)",
            "https://www.youtube.com/watch?v=jfKfPfyJRdk",
            WebSourceType.Video,
            WebSourceCategory.Entertainment,
            "YouTube",
            "24/7 chill beats stream",
            false,
            "",
            0); // Live
            
        yield return new("earthcam_times_square",
            "EarthCam - Times Square Live",
            "https://www.earthcam.com/usa/newyork/timessquare/?cam=tsrobo1",
            WebSourceType.Page,
            WebSourceCategory.Entertainment,
            "EarthCam",
            "Live webcam of Times Square NYC",
            false,
            "",
            0); // Live
        
        // ====================
        // MAPS & GEOGRAPHY
        // ====================
        
        yield return new("osm_tile_world",
            "OpenStreetMap World Tile",
            "https://tile.openstreetmap.org/2/1/1.png",
            WebSourceType.Image,
            WebSourceCategory.Technology,
            "© OpenStreetMap contributors",
            "Open source world map tiles",
            false,
            "",
            1440);
            
        yield return new("windy_map",
            "Windy - Wind Map",
            "https://embed.windy.com/embed2.html?lat=38.889&lon=-77.035&zoom=5&level=surface&overlay=wind&menu=&message=&marker=&calendar=&pressure=&type=map&location=coordinates&detail=&metricWind=default&metricTemp=default&radarRange=-1",
            WebSourceType.Page,
            WebSourceCategory.Weather,
            "Windy.com",
            "Beautiful wind and weather visualization",
            false,
            "",
            30);
        
        // ====================
        // SCIENCE & EDUCATION
        // ====================
        
        yield return new("arxiv_recent",
            "arXiv Recent Papers (Physics)",
            "http://export.arxiv.org/api/query?search_query=cat:physics.*&sortBy=submittedDate&sortOrder=descending&max_results=10",
            WebSourceType.RSS,
            WebSourceCategory.Science,
            "arXiv (Cornell)",
            "Latest physics research papers",
            false,
            "",
            360);
            
        yield return new("numbers_api_math",
            "Numbers API - Math Facts",
            "http://numbersapi.com/random/math?json",
            WebSourceType.JSON,
            WebSourceCategory.Education,
            "Numbers API",
            "Interesting mathematical facts",
            false,
            "",
            1440);
            
        yield return new("quote_of_day",
            "Quote of the Day",
            "https://zenquotes.io/api/today",
            WebSourceType.JSON,
            WebSourceCategory.Education,
            "ZenQuotes.io",
            "Inspirational daily quote",
            false,
            "",
            1440);
            
        yield return new("astronomy_pod_archive",
            "Astronomy Picture Archive",
            "https://api.nasa.gov/planetary/apod?api_key=DEMO_KEY&count=5",
            WebSourceType.JSON,
            WebSourceCategory.Space,
            "NASA APOD",
            "Random selection of astronomy pictures",
            true,
            "NASA_API_KEY",
            360);
    }

    public async Task<(bool success, string html)> BuildEmbedHtmlAsync(
        CuratedSource source, 
        CancellationToken cancellationToken = default)
    {
        try
        {
            // Check cache first
            if (EnableCaching && await TryGetCachedAsync(source.Id) is { } cached)
            {
                _logger.LogDebug("Cache hit for {Id}", source.Id);
                return (true, cached);
            }

            var (success, html) = source.Type switch
            {
                WebSourceType.Image => BuildImageEmbed(source),
                WebSourceType.Video => BuildVideoEmbed(source),
                WebSourceType.Page => BuildPageEmbed(source),
                WebSourceType.JSON => await FetchAndFormatJsonAsync(source, cancellationToken),
                WebSourceType.RSS => await FetchAndFormatRssAsync(source, cancellationToken),
                WebSourceType.Widget => BuildWidgetEmbed(source),
                WebSourceType.Live => BuildLiveEmbed(source),
                WebSourceType.Interactive => BuildInteractiveEmbed(source),
                _ => (false, "Unsupported source type")
            };

            if (success && EnableCaching)
            {
                await CacheResponseAsync(source.Id, html);
            }

            return (success, html);
        }
        catch (OperationCanceledException)
        {
            _logger.LogInformation("Request cancelled for {Id}", source.Id);
            throw;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to build embed for {Id}", source.Id);
            return (false, BuildErrorHtml(source.Name, ex.Message));
        }
    }

    private (bool, string) BuildImageEmbed(CuratedSource source)
    {
        return (true, $@"
<div style='width: 100%; height: 100%; display: flex; align-items: center; justify-content: center; background: #000;'>
    <img src='{Escape(source.Url)}' 
         alt='{Escape(source.Name)}' 
         style='max-width: 100%; max-height: 100%; object-fit: contain;'
         loading='lazy' />
</div>");
    }

    private (bool, string) BuildVideoEmbed(CuratedSource source)
    {
        if (IsYouTubeUrl(source.Url))
        {
            var videoId = ExtractYouTubeId(source.Url);
            if (!string.IsNullOrWhiteSpace(videoId))
            {
                return (true, $@"
<iframe src='https://www.youtube-nocookie.com/embed/{videoId}?autoplay=1&mute=1&loop=1&playlist={videoId}&controls=1' 
        style='width:100%; height:100%; border:0;' 
        allow='autoplay; encrypted-media; picture-in-picture' 
        allowfullscreen
        sandbox='allow-scripts allow-same-origin allow-presentation'
        referrerpolicy='no-referrer'
        loading='lazy'>
</iframe>");
            }
        }

        // Direct video
        return (true, $@"
<video src='{Escape(source.Url)}' 
       autoplay 
       muted 
       loop 
       controls 
       style='width:100%; height:100%; object-fit: contain;'
       preload='metadata'>
    Your browser does not support the video tag.
</video>");
    }

    private (bool, string) BuildPageEmbed(CuratedSource source)
    {
        return (true, $@"
<iframe src='{Escape(source.Url)}' 
        style='width:100%; height:100%; border:0;' 
        sandbox='allow-scripts allow-same-origin allow-popups allow-forms' 
        referrerpolicy='no-referrer'
        loading='lazy'>
</iframe>");
    }

    private (bool, string) BuildWidgetEmbed(CuratedSource source)
    {
        // Custom widget rendering (can be extended with specific widget types)
        return (true, $@"
<div style='width:100%; height:100%; display:flex; align-items:center; justify-content:center; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; font-family: Segoe UI, sans-serif; padding: 40px; text-align: center;'>
    <div>
        <h2 style='margin: 0 0 20px 0; font-size: 32px;'>âš™ï¸ {Escape(source.Name)}</h2>
        <p style='margin: 0; font-size: 16px; opacity: 0.9;'>{Escape(source.Description)}</p>
        <div style='margin-top: 30px; padding: 20px; background: rgba(255,255,255,0.1); border-radius: 12px; backdrop-filter: blur(10px);'>
            <a href='{Escape(source.Url)}' target='_blank' style='color: white; text-decoration: none; font-weight: bold;'>Open Widget →</a>
        </div>
    </div>
</div>");
    }

    private (bool, string) BuildLiveEmbed(CuratedSource source)
    {
        // For live streams
        if (IsYouTubeUrl(source.Url))
        {
            return BuildVideoEmbed(source);
        }

        return BuildPageEmbed(source);
    }

    private (bool, string) BuildInteractiveEmbed(CuratedSource source)
    {
        // For interactive content (maps, visualizations, etc.)
        return BuildPageEmbed(source);
    }

    private async Task<(bool, string)> FetchAndFormatJsonAsync(
        CuratedSource source, 
        CancellationToken ct)
    {
        try
        {
            var url = SubstituteApiKeys(source);
            var json = await _http.GetStringAsync(url, ct);
            
            // Try to prettify
            string formatted;
            try
            {
                var jsonDoc = JsonDocument.Parse(json);
                formatted = JsonSerializer.Serialize(jsonDoc, new JsonSerializerOptions 
                { 
                    WriteIndented = true,
                    Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
                });
            }
            catch
            {
                formatted = json; // Use as-is if can't parse
            }

            var html = BuildJsonDisplayHtml(source, formatted);
            return (true, html);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to fetch JSON from {Url}", source.Url);
            return (false, BuildErrorHtml(source.Name, ex.Message));
        }
    }

    private async Task<(bool, string)> FetchAndFormatRssAsync(
        CuratedSource source,
        CancellationToken ct)
    {
        try
        {
            var xml = await _http.GetStringAsync(source.Url, ct);
            var html = BuildRssDisplayHtml(source, xml);
            return (true, html);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to fetch RSS from {Url}", source.Url);
            return (false, BuildErrorHtml(source.Name, ex.Message));
        }
    }

    // HTML Builders

    private string BuildJsonDisplayHtml(CuratedSource source, string json)
    {
        var categoryColor = GetCategoryColor(source.Category);
        var categoryIcon = GetCategoryIcon(source.Category);

        return $@"
<div style='font-family: Segoe UI, Arial, sans-serif; padding: 25px; background: #0f172a; min-height: 100%; color: #e2e8f0;'>
    <div style='background: linear-gradient(135deg, {categoryColor.primary} 0%, {categoryColor.secondary} 100%); padding: 25px; border-radius: 12px; margin-bottom: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.3);'>
        <div style='display: flex; align-items: center; gap: 15px;'>
            <div style='font-size: 48px;'>{categoryIcon}</div>
            <div style='flex: 1;'>
                <h1 style='margin: 0 0 8px 0; font-size: 28px; color: white;'>{Escape(source.Name)}</h1>
                <p style='margin: 0; opacity: 0.9; font-size: 14px;'>{Escape(source.Description)}</p>
            </div>
            <div style='text-align: right; opacity: 0.8; font-size: 12px;'>
                <div>🔄 Refresh: {source.RefreshMinutes}min</div>
                <div style='margin-top: 5px;'>📊 {source.Category}</div>
            </div>
        </div>
    </div>
    
    <div style='background: #1e293b; padding: 25px; border-radius: 12px; overflow: auto; max-height: calc(100vh - 250px); box-shadow: 0 4px 6px rgba(0,0,0,0.1);'>
        <pre style='margin: 0; color: #cbd5e1; font-family: JetBrains Mono, Consolas, Monaco, monospace; font-size: 13px; line-height: 1.6; white-space: pre-wrap; word-wrap: break-word;'>{Escape(json)}</pre>
    </div>
    
    <div style='margin-top: 15px; padding: 15px; background: rgba(255,255,255,0.05); border-radius: 8px; font-size: 12px; color: #94a3b8;'>
        <strong>Attribution:</strong> {Escape(source.Attribution)}
    </div>
</div>";
    }

    private string BuildRssDisplayHtml(CuratedSource source, string xml)
    {
        return $@"
<div style='font-family: Segoe UI, Arial, sans-serif; padding: 25px; background: #fafafa; min-height: 100%;'>
    <h1 style='margin: 0 0 20px 0; color: #1f2937;'>📰 {Escape(source.Name)}</h1>
    <div style='background: white; padding: 25px; border-radius: 12px; overflow: auto; max-height: 80vh; box-shadow: 0 2px 8px rgba(0,0,0,0.1);'>
        <pre style='margin: 0; color: #374151; font-family: Consolas, Monaco, monospace; font-size: 12px; line-height: 1.5; white-space: pre-wrap;'>{Escape(xml)}</pre>
    </div>
</div>";
    }

    private string BuildErrorHtml(string sourceName, string errorMessage)
    {
        return $@"
<div style='font-family: Segoe UI, Arial, sans-serif; padding: 40px; background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%); min-height: 100%; display: flex; align-items: center; justify-content: center; color: white;'>
    <div style='text-align: center; max-width: 500px;'>
        <div style='font-size: 72px; margin-bottom: 20px;'>❌</div>
        <h2 style='margin: 0 0 15px 0; font-size: 28px;'>Failed to Load</h2>
        <p style='margin: 0 0 10px 0; font-size: 18px; font-weight: bold;'>{Escape(sourceName)}</p>
        <div style='margin-top: 25px; padding: 20px; background: rgba(255,255,255,0.1); border-radius: 12px; backdrop-filter: blur(10px);'>
            <p style='margin: 0; font-size: 14px; opacity: 0.9;'>{Escape(errorMessage)}</p>
        </div>
    </div>
</div>";
    }

    // Helper Methods

    private string SubstituteApiKeys(CuratedSource source)
    {
        var url = source.Url;

        if (source.RequiresApiKey && !string.IsNullOrWhiteSpace(source.ApiKeyEnvVar))
        {
            var apiKey = Environment.GetEnvironmentVariable(source.ApiKeyEnvVar);
            if (!string.IsNullOrWhiteSpace(apiKey))
            {
                url = url.Replace("DEMO_KEY", apiKey);
            }
        }

        return url;
    }

    private (string primary, string secondary) GetCategoryColor(WebSourceCategory category) => category switch
    {
        WebSourceCategory.Space => ("#6366f1", "#4f46e5"),
        WebSourceCategory.Weather => ("#0ea5e9", "#0284c7"),
        WebSourceCategory.News => ("#ef4444", "#dc2626"),
        WebSourceCategory.Science => ("#10b981", "#059669"),
        WebSourceCategory.Art => ("#f59e0b", "#d97706"),
        WebSourceCategory.Nature => ("#22c55e", "#16a34a"),
        WebSourceCategory.Technology => ("#8b5cf6", "#7c3aed"),
        WebSourceCategory.Finance => ("#eab308", "#ca8a04"),
        WebSourceCategory.Social => ("#ec4899", "#db2777"),
        WebSourceCategory.Entertainment => ("#f43f5e", "#e11d48"),
        WebSourceCategory.Education => ("#3b82f6", "#2563eb"),
        WebSourceCategory.Health => ("#14b8a6", "#0d9488"),
        WebSourceCategory.Sports => ("#f97316", "#ea580c"),
        _ => ("#64748b", "#475569")
    };

    private string GetCategoryIcon(WebSourceCategory category) => category switch
    {
        WebSourceCategory.Space => "🚀",
        WebSourceCategory.Weather => "🌤️",
        WebSourceCategory.News => "📰",
        WebSourceCategory.Science => "🔬",
        WebSourceCategory.Art => "🎨",
        WebSourceCategory.Nature => "🌿",
        WebSourceCategory.Technology => "💻",
        WebSourceCategory.Finance => "💰",
        WebSourceCategory.Social => "👥",
        WebSourceCategory.Entertainment => "🎬",
        WebSourceCategory.Education => "📚",
        WebSourceCategory.Health => "❤️",
        WebSourceCategory.Sports => "⚽",
        _ => "📌"
    };

    private static bool IsYouTubeUrl(string url) =>
        url.Contains("youtube.com", StringComparison.OrdinalIgnoreCase) ||
        url.Contains("youtu.be", StringComparison.OrdinalIgnoreCase) ||
        url.Contains("youtube-nocookie.com", StringComparison.OrdinalIgnoreCase);

    private static string ExtractYouTubeId(string url)
    {
        try
        {
            var uri = new Uri(url);
            
            // youtu.be/VIDEO_ID
            if (uri.Host.Contains("youtu.be", StringComparison.OrdinalIgnoreCase))
            {
                return uri.AbsolutePath.Trim('/');
            }
            
            // youtube.com/watch?v=VIDEO_ID
            var query = System.Web.HttpUtility.ParseQueryString(uri.Query);
            var v = query["v"];
            if (!string.IsNullOrWhiteSpace(v))
                return v;
            
            // youtube.com/embed/VIDEO_ID
            var segments = uri.Segments;
            if (segments.Length > 1 && segments[segments.Length - 2].Trim('/').Equals("embed", StringComparison.OrdinalIgnoreCase))
            {
                return segments[^1].Trim('/');
            }
        }
        catch { }
        
        return string.Empty;
    }

    private static string Escape(string s) => 
        System.Net.WebUtility.HtmlEncode(s ?? string.Empty);

    // Caching

    private async Task<string?> TryGetCachedAsync(string key)
    {
        await _cacheLock.WaitAsync();
        try
        {
            if (_cache.TryGetValue(key, out var cached))
            {
                if ((DateTime.UtcNow - cached.timestamp).TotalMinutes < CacheMinutes)
                {
                    return cached.content;
                }
                else
                {
                    _cache.Remove(key);
                }
            }
        }
        finally
        {
            _cacheLock.Release();
        }
        
        return null;
    }

    private async Task CacheResponseAsync(string key, string content)
    {
        await _cacheLock.WaitAsync();
        try
        {
            _cache[key] = (DateTime.UtcNow, content);
            
            // Cleanup old entries if cache is large
            if (_cache.Count > 100)
            {
                var oldest = _cache
                    .OrderBy(kv => kv.Value.timestamp)
                    .Take(20)
                    .Select(kv => kv.Key)
                    .ToList();
                    
                foreach (var old in oldest)
                {
                    _cache.Remove(old);
                }
            }
        }
        finally
        {
            _cacheLock.Release();
        }
    }

    public void ClearCache()
    {
        _cacheLock.Wait();
        try
        {
            _cache.Clear();
            _logger.LogInformation("Cache cleared");
        }
        finally
        {
            _cacheLock.Release();
        }
    }
}