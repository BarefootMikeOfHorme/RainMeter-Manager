using Microsoft.Extensions.Logging;

namespace RenderProcess.Content;

public class APIContentLoader
{
    private readonly ILogger<APIContentLoader> _logger;
    public APIContentLoader(ILogger<APIContentLoader> logger) { _logger = logger; }
}

