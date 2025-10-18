# RainmeterManager Content Loaders

Comprehensive content loading system for the RenderProcess with support for 50+ file types, web content, media, and dynamic APIs.

## 📦 Components

### Content Loaders
- **FileContentLoader** (46 KB) - Handles 50+ file formats with syntax highlighting, thumbnails, and metadata extraction
- **WebContentLoader** (34 KB) - Loads web content from 50+ curated sources with caching and rate limiting
- **MediaContentLoader** (15 KB) - Processes video/audio with metadata extraction
- **APIContentLoader** (40 KB) - Dynamic REST API loader with authentication and environment management

### Management Scripts
- **deploy_loaders.ps1** - Enterprise-grade deployment with integrity checks
- **verify_loaders.ps1** - Health check and validation
- **configure_loaders.ps1** - Secure configuration management

## 🚀 Quick Start

### 1. Deploy Content Loaders

```powershell
# Dry run first (recommended)
.\deploy_loaders.ps1 -DryRun

# Deploy with integrity checks
.\deploy_loaders.ps1 -Force

# Non-interactive mode for CI/CD
.\deploy_loaders.ps1 -Force -NonInteractive
```

**Parameters:**
- `-TargetPath` - Deployment destination (default: auto-detected)
- `-Backup` - Create timestamped backup (default: $true)
- `-Force` - Skip overwrite prompts
- `-DryRun` - Preview without making changes
- `-NonInteractive` - No prompts (for automation)
- `-VerifyIntegrity` - SHA256 verification (default: $true)
- `-SkipPostVerification` - Skip post-deployment checks
- `-MaxBackups` - Keep N most recent backups (default: 5)

### 2. Verify Installation

```powershell
# Basic verification
.\verify_loaders.ps1

# Verbose output with class detection
.\verify_loaders.ps1 -Verbose

# Skip manifest checks
.\verify_loaders.ps1 -CheckManifest:$false
```

### 3. Setup API Keys (IMPORTANT)

```powershell
# Interactive wizard - automatically opens signup pages
.\setup_api_keys.ps1

# Make keys permanent across sessions
.\setup_api_keys.ps1 -SetPermanent

# Read detailed guide
code API_KEY_GUIDE.md
```

**Priority APIs** (Free, instant approval):
- ⭐⭐⭐ NASA API - 7 space sources (1000 req/hour)
- ⭐⭐⭐ WAQI Air Quality - Real-time air quality
- ⭐⭐ WeatherAPI.com - Weather forecasts

### 4. Configure Loaders (Optional)

```powershell
# Generate configuration template
.\configure_loaders.ps1 -GenerateTemplate

# Interactive configuration
.\configure_loaders.ps1 -Interactive

# Validate existing config
.\configure_loaders.ps1 -ValidateOnly
```

## ⚙️ Configuration

### appsettings.json Structure

```json
{
  "ContentLoaders": {
    "FileContentLoader": {
      "MaxFileBytes": 67108864,
      "EnableThumbnails": true,
      "EnableSyntaxHighlighting": true
    },
    "WebContentLoader": {
      "TimeoutSeconds": 30,
      "EnableCaching": true,
      "ApiKeys": {
        "OpenWeatherMap": "<YOUR_KEY>",
        "NewsAPI": "<YOUR_KEY>",
        "GitHub": "<YOUR_KEY>"
      }
    },
    "MediaContentLoader": {
      "MaxVideoSizeBytes": 524288000,
      "EnableMetadataExtraction": true
    },
    "APIContentLoader": {
      "TimeoutSeconds": 30,
      "EnableResponseCaching": true
    }
  }
}
```

### API Keys Required

**WebContentLoader** supports these services:
- OpenWeatherMap - Weather data
- NewsAPI - News headlines
- GitHub - Repository data
- Twitter/X - Social media (optional)
- Reddit - Forum content (optional)

**Obtaining API Keys:**
1. OpenWeatherMap: https://openweathermap.org/api
2. NewsAPI: https://newsapi.org/register
3. GitHub: https://github.com/settings/tokens

## 🔒 Security Features

### Deployment Script
- ✅ SHA256 integrity verification
- ✅ Automatic backups with rotation
- ✅ Post-deployment validation
- ✅ Non-interactive mode for CI/CD
- ✅ Rollback capability via backups
- ✅ Detailed logging and error handling

### Configuration
- ⚠️ API keys stored in appsettings.json (development)
- 🔐 Production: Use secure credential storage (e.g., Azure Key Vault, Windows Credential Manager)
- 🔒 Consider environment variables for sensitive values

## 🏗️ Integration with RenderProcess

The loaders are **already registered** in `Program.cs`:

```csharp
services.AddTransient<FileContentLoader>();
services.AddTransient<WebContentLoader>();
services.AddTransient<MediaContentLoader>();
services.AddTransient<APIContentLoader>();
```

### Using Loaders in Your Code

```csharp
public class MyRenderComponent
{
    private readonly FileContentLoader _fileLoader;
    
    public MyRenderComponent(FileContentLoader fileLoader)
    {
        _fileLoader = fileLoader;
    }
    
    public async Task LoadContent(string path)
    {
        using var result = await _fileLoader.LoadFileAsync(path);
        
        if (result.ContentType == FileContentType.Image)
        {
            // Use result.Image or result.Thumbnail
        }
        else if (result.ContentType == FileContentType.Code)
        {
            // Use result.Html for syntax-highlighted code
        }
    }
}
```

## 📊 Supported File Types

### FileContentLoader
**Code:** C#, Python, JavaScript, TypeScript, C++, Java, Go, Rust, PHP, Ruby, Swift, Lua, SQL, PowerShell, Bash  
**Documents:** TXT, MD, RST, PDF, DOC, DOCX, RTF  
**Data:** JSON, YAML, TOML, XML, CSV, TSV  
**Web:** HTML, XHTML, SVG  
**Media:** PNG, JPG, GIF, BMP, WEBP, ICO, TIFF  
**Video:** MP4, AVI, MKV, MOV, WEBM, FLV  
**Audio:** MP3, WAV, FLAC, OGG, M4A, AAC  
**Archives:** ZIP, RAR, 7Z, TAR, GZ, BZ2  
**Fonts:** TTF, OTF, WOFF, WOFF2

### WebContentLoader
50+ curated sources including: Weather, News, GitHub, Reddit, Twitter, RSS feeds, RESTful APIs

## 🔧 Troubleshooting

### Deployment Issues

**"Missing source files"**
- Ensure you're in the `renderprocess\Content` directory
- All 4 .cs files must be present in same directory as deploy script

**"Hash mismatch"**
- Source files may be corrupted
- Try re-downloading or restoring from backup
- Use `-VerifyIntegrity:$false` to skip checks (not recommended)

### Verification Failures

**"Loader not registered in Program.cs"**
- Check that Program.cs hasn't been modified
- Ensure all `services.AddTransient<>` lines are present

**"Hash mismatch: file may have been modified"**
- Normal if you've edited loaders after deployment
- Redeploy to update manifest hashes

### Configuration Issues

**"API key still has placeholder value"**
- Run `.\configure_loaders.ps1 -Interactive` to set keys
- Or manually edit appsettings.json

**"Failed to parse configuration"**
- JSON syntax error in appsettings.json
- Use https://jsonlint.com to validate
- Restore from `.backup_*` file if available

## 📁 File Structure

```
renderprocess/
├── Content/
│   ├── FileContentLoader.cs           (46 KB)
│   ├── WebContentLoader.cs            (34 KB)
│   ├── MediaContentLoader.cs          (15 KB)
│   ├── APIContentLoader.cs            (40 KB)
│   ├── deploy_loaders.ps1             (Enhanced v3.0)
│   ├── verify_loaders.ps1             (Health check)
│   ├── configure_loaders.ps1          (Config manager)
│   ├── deployment_manifest.json       (Generated)
│   └── README.md                      (This file)
├── Program.cs                         (DI registration)
├── appsettings.json                   (Configuration)
└── RenderProcess.csproj               (Project file)
```

## 🔄 Workflow Examples

### Initial Setup
```powershell
# 1. Deploy loaders
.\deploy_loaders.ps1 -Force

# 2. Verify deployment
.\verify_loaders.ps1

# 3. Generate config template
.\configure_loaders.ps1 -GenerateTemplate

# 4. Configure API keys
.\configure_loaders.ps1 -Interactive
```

### CI/CD Pipeline
```powershell
# Automated deployment
.\deploy_loaders.ps1 -Force -NonInteractive -Backup:$false

# Verify without manifest check (first deploy)
.\verify_loaders.ps1 -CheckManifest:$false

# Validate config
.\configure_loaders.ps1 -ValidateOnly
```

### Development Workflow
```powershell
# Make changes to loaders

# Test deployment
.\deploy_loaders.ps1 -DryRun

# Deploy
.\deploy_loaders.ps1 -Force

# Verify
.\verify_loaders.ps1 -Verbose
```

### Rollback Procedure
```powershell
# List available backups
Get-ChildItem backup_content_loaders_* -Directory | Sort-Object CreationTime -Descending

# Restore from backup
$backupDir = "backup_content_loaders_20251019_013000"
Copy-Item "$backupDir\*.cs" "D:\RainmeterManager\renderprocess\Content" -Force

# Verify restoration
.\verify_loaders.ps1
```

## 📚 Documentation

- **Complete Documentation**: See `Enhanced Content Loaders - Complete Documentation.pdf` (2 MB)
- **Package Details**: See `Content Loaders Complete Package.pdf` (1.2 MB)

## 🆘 Support

For issues or questions:
1. Run `.\verify_loaders.ps1 -Verbose` and review output
2. Check deployment manifest at `deployment_manifest.json`
3. Review logs in RenderProcess output
4. Check backup directories for rollback options

## 📝 Version History

### v3.0.0 (Current)
- Enhanced deployment with SHA256 integrity checks
- Non-interactive mode for CI/CD
- Automated backup rotation (keeps 5 most recent)
- Post-deployment verification
- Configuration management script
- Comprehensive verification script

### v2.0.0 (Previous)
- Initial production release
- Basic deployment script
- Manual configuration

## 🎯 Next Steps

After successful deployment:
1. ✅ Rebuild RenderProcess project
2. ✅ Test with sample content files
3. ✅ Configure API keys for web sources
4. ✅ Integrate loaders in your render components
5. ✅ Monitor logs for any issues

---

**Status**: ✅ Production Ready  
**Last Updated**: 2025-10-19  
**Maintainer**: RainmeterManager Team
