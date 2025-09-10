using Microsoft.Extensions.Logging;
using SkiaSharp;
using System;
using System.Buffers;
using System.IO;
using System.Net;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace RenderProcess.Content;

public enum FileContentType
{
    Unsupported,
    Html,
    Text,
    Json,
    Xml,
    Image,
    Binary
}

public sealed class FileLoadResult : IDisposable
{
    public FileContentType ContentType { get; init; } = FileContentType.Unsupported;
    public string? Text { get; init; }
    public string? Html { get; init; }
    public byte[]? Bytes { get; init; }
    public SKBitmap? Image { get; init; }
    public string? MimeType { get; init; }

    public void Dispose()
    {
        Image?.Dispose();
    }
}

/// <summary>
/// Secure file loader for the RenderProcess. Handles text/JSON/XML/html, images, and small binaries.
/// Applies size limits, encoding detection, and safe HTML preview building when appropriate.
/// </summary>
public class FileContentLoader
{
    private readonly ILogger<FileContentLoader> _logger;

    // Hard safety caps
    private const long MaxFileBytes = 64L * 1024 * 1024; // 64 MB

    public FileContentLoader(ILogger<FileContentLoader> logger)
    {
        _logger = logger;
    }

    public bool IsSupported(string path) => GetContentTypeByExtension(path) != FileContentType.Unsupported;

    public async Task<FileLoadResult> LoadAsync(string path)
    {
        var result = new FileLoadResult();
        try
        {
            if (string.IsNullOrWhiteSpace(path) || !File.Exists(path))
            {
                _logger.LogWarning("FileContentLoader: path missing or not found: {path}", path);
                return result;
            }

            var fileInfo = new FileInfo(path);
            if (fileInfo.Length > MaxFileBytes)
            {
                _logger.LogWarning("FileContentLoader: file too large ({bytes} bytes): {path}", fileInfo.Length, path);
                return result;
            }

            var type = GetContentTypeByExtension(path);

            if (type == FileContentType.Unsupported)
            {
                // Sniff content to guess
                type = await SniffContentTypeAsync(path).ConfigureAwait(false);
            }

            switch (type)
            {
                case FileContentType.Image:
                    return await LoadImageAsync(path).ConfigureAwait(false);
                case FileContentType.Html:
                    return await LoadHtmlAsync(path).ConfigureAwait(false);
                case FileContentType.Json:
                case FileContentType.Xml:
                case FileContentType.Text:
                    return await LoadTextLikeAsync(path, type).ConfigureAwait(false);
                case FileContentType.Binary:
                    return await LoadBinaryAsync(path).ConfigureAwait(false);
                default:
                    // Fallback: try text, else binary
                    if (await LooksLikeTextAsync(path).ConfigureAwait(false))
                        return await LoadTextLikeAsync(path, FileContentType.Text).ConfigureAwait(false);
                    return await LoadBinaryAsync(path).ConfigureAwait(false);
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "FileContentLoader: exception loading {path}", path);
            return result;
        }
    }

    private static FileContentType GetContentTypeByExtension(string path)
    {
        var ext = Path.GetExtension(path).ToLowerInvariant();
        return ext switch
        {
            ".htm" or ".html" => FileContentType.Html,
            ".txt" => FileContentType.Text,
            ".json" => FileContentType.Json,
            ".xml" => FileContentType.Xml,
            ".png" or ".jpg" or ".jpeg" or ".gif" or ".bmp" or ".webp" => FileContentType.Image,
            ".pdf" => FileContentType.Binary,
            ".zip" => FileContentType.Binary,
            ".bin" => FileContentType.Binary,
            _ => FileContentType.Unsupported
        };
    }

    private async Task<FileContentType> SniffContentTypeAsync(string path)
    {
        try
        {
            byte[] head = new byte[12];
            using var fs = File.Open(path, FileMode.Open, FileAccess.Read, FileShare.Read);
            int read = await fs.ReadAsync(head, 0, head.Length).ConfigureAwait(false);
            if (read >= 4)
            {
                // PNG
                if (read >= 8 && head[0] == 0x89 && head[1] == 0x50 && head[2] == 0x4E && head[3] == 0x47)
                    return FileContentType.Image;
                // JPEG
                if (head[0] == 0xFF && head[1] == 0xD8)
                    return FileContentType.Image;
                // GIF
                if (read >= 6 && head[0] == 0x47 && head[1] == 0x49 && head[2] == 0x46)
                    return FileContentType.Image;
                // WEBP (RIFF....WEBP)
                if (read >= 12 && head[0] == 0x52 && head[1] == 0x49 && head[2] == 0x46 && head[3] == 0x46 && head[8] == 0x57 && head[9] == 0x45 && head[10] == 0x42 && head[11] == 0x50)
                    return FileContentType.Image;
                // PDF
                if (head[0] == 0x25 && head[1] == 0x50 && head[2] == 0x44 && head[3] == 0x46)
                    return FileContentType.Binary;
                // ZIP
                if (head[0] == 0x50 && head[1] == 0x4B)
                    return FileContentType.Binary;
            }
            // heuristics: treat as text if no null bytes in first N bytes
            if (await LooksLikeTextAsync(path).ConfigureAwait(false))
                return FileContentType.Text;
        }
        catch { }
        return FileContentType.Binary;
    }

    private static async Task<bool> LooksLikeTextAsync(string path)
    {
        const int N = 5120;
        using var fs = File.Open(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        byte[] buf = new byte[N];
        int read = await fs.ReadAsync(buf, 0, buf.Length).ConfigureAwait(false);
        for (int i = 0; i < read; i++)
        {
            if (buf[i] == 0x00) return false;
        }
        return true;
    }

    private async Task<FileLoadResult> LoadTextLikeAsync(string path, FileContentType type)
    {
        // Try to detect encoding via BOM; default to UTF-8
        string text;
        using (var fs = File.Open(path, FileMode.Open, FileAccess.Read, FileShare.Read))
        using (var reader = new StreamReader(fs, DetectEncodingFromBom(fs) ?? new UTF8Encoding(encoderShouldEmitUTF8Identifier: false), detectEncodingFromByteOrderMarks: true))
        {
            text = await reader.ReadToEndAsync().ConfigureAwait(false);
        }

        // Build a safe HTML preview for WebView2 or other consumers
        var preview = BuildHtmlPre(text, Path.GetFileName(path), type);
        string mime = type switch
        {
            FileContentType.Json => "application/json",
            FileContentType.Xml => "application/xml",
            _ => "text/plain"
        };

        return new FileLoadResult
        {
            ContentType = type,
            Text = text,
            Html = preview,
            MimeType = mime
        };
    }

    private async Task<FileLoadResult> LoadHtmlAsync(string path)
    {
        // For .html, we return the raw HTML as-is and a sanitized preview variant (optional).
        string html = await File.ReadAllTextAsync(path).ConfigureAwait(false);
        return new FileLoadResult
        {
            ContentType = FileContentType.Html,
            Html = html,
            MimeType = "text/html"
        };
    }

    private async Task<FileLoadResult> LoadBinaryAsync(string path)
    {
        var bytes = await File.ReadAllBytesAsync(path).ConfigureAwait(false);
        return new FileLoadResult
        {
            ContentType = FileContentType.Binary,
            Bytes = bytes,
            MimeType = "application/octet-stream"
        };
    }

    private async Task<FileLoadResult> LoadImageAsync(string path)
    {
        try
        {
            using var fs = File.OpenRead(path);
            using var ms = new MemoryStream();
            await fs.CopyToAsync(ms).ConfigureAwait(false);
            ms.Position = 0;

            // Decode using SkiaSharp
            using var skStream = new SKManagedStream(ms);
            var bitmap = SKBitmap.Decode(skStream);
            if (bitmap == null)
            {
                _logger.LogWarning("FileContentLoader: failed to decode image {path}", path);
                return new FileLoadResult();
            }

            return new FileLoadResult
            {
                ContentType = FileContentType.Image,
                Image = bitmap,
                MimeType = GuessImageMime(Path.GetExtension(path))
            };
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "FileContentLoader: image decode error {path}", path);
            return new FileLoadResult();
        }
    }

    private static string BuildHtmlPre(string text, string title, FileContentType type)
    {
        var safe = WebUtility.HtmlEncode(text ?? string.Empty);
        var label = type switch
        {
            FileContentType.Json => "JSON",
            FileContentType.Xml => "XML",
            _ => title
        };
        return $@"
<div style='font-family: Consolas, monospace;'>
  <h3 style='color: #4a9eff; margin-top: 0;'>{WebUtility.HtmlEncode(label)}</h3>
  <div style='background: #2a2a2a; padding: 15px; border-radius: 8px; overflow: auto;'>
    <pre style='color: #e0e0e0; margin: 0; white-space: pre-wrap;'>{safe}</pre>
  </div>
</div>";
    }

    private static Encoding? DetectEncodingFromBom(Stream stream)
    {
        if (!stream.CanSeek) return null;
        var original = stream.Position;
        Span<byte> bom = stackalloc byte[4];
        int read = stream.Read(bom);
        stream.Position = original;
        if (read >= 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) return new UTF8Encoding(true);
        if (read >= 2 && bom[0] == 0xFF && bom[1] == 0xFE) return Encoding.Unicode; // UTF-16 LE
        if (read >= 2 && bom[0] == 0xFE && bom[1] == 0xFF) return Encoding.BigEndianUnicode; // UTF-16 BE
        if (read >= 4 && bom[0] == 0x00 && bom[1] == 0x00 && bom[2] == 0xFE && bom[3] == 0xFF) return Encoding.GetEncoding("utf-32BE");
        if (read >= 4 && bom[0] == 0xFF && bom[1] == 0xFE && bom[2] == 0x00 && bom[3] == 0x00) return Encoding.UTF32; // UTF-32 LE
        return null;
    }

    private static string GuessImageMime(string ext)
    {
        switch (ext.ToLowerInvariant())
        {
            case ".png": return "image/png";
            case ".jpg":
            case ".jpeg": return "image/jpeg";
            case ".gif": return "image/gif";
            case ".bmp": return "image/bmp";
            case ".webp": return "image/webp";
            default: return "image/*";
        }
    }
}

