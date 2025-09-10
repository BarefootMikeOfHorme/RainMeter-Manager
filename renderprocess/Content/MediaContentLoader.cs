using Microsoft.Extensions.Logging;

namespace RenderProcess.Content;

public class MediaContentLoader
{
    private readonly ILogger<MediaContentLoader> _logger;
    public MediaContentLoader(ILogger<MediaContentLoader> logger) { _logger = logger; }
}

