# AXIOM v2.0 - Automated eXtraction Index Operator

Enterprise-grade Rainmeter skin scraper with SQLite database, async downloads, and security features.

## 🚀 Quick Start

```bash
# Install dependencies
pip install -r requirements.txt

# Run with defaults
python AXIOM.py

# Fast mode
python AXIOM.py --workers 10 --delay 0.5

# Custom output directory
python AXIOM.py --output my_collection --batch-size 200
```

## 📦 What's Included

### Main Files
- **AXIOM.py** (55KB, 1394 lines) - Complete production-ready scraper
  - SQLite database storage
  - Async downloads with aiohttp
  - Adaptive rate limiting
  - Secure extraction (path traversal protection)
  - Resume capability
  - Startup branding/animations (pygame)
  
- **axiom_utilities.py** - Database management CLI
  - `stats` - View collection statistics
  - `export` - Export to JSON/CSV/TXT
  - `search` - Search skins by field
  - `reset` - Reset download statuses
  - `cleanup` - Remove orphaned files
  - `backup` - Create database backups
  - `check` - Integrity verification
  - `vacuum` - Compact database

- **axiom_tests.py** - Comprehensive test suite

### Documentation
- **AXIOM v2.0 - Complete Documentation.pdf** - Full reference guide
- **AXIOM Quick Reference & Cheat Sheet.pdf** - Quick lookup
- **axiom_documentation.md** - Markdown documentation
- **axiom_quick_reference.md** - Command reference

### Scripts
- **setup_and_run.bat** - Windows launcher with auto-setup
- **enterprise_scraper.bat** - Enterprise batch script

## 🏗️ Architecture

### v2.0 Consolidation (October 2025)

**Before**: 3 fragmented files
- `AXIOM.py` (1740 lines) - Old monolithic version with pygame bloat
- `axiom_enhanced.py` (737 lines) - Incomplete async infrastructure
- `axiom_scraper_core.py` - Separate scraper logic

**After**: 1 production-ready file
- `AXIOM.py` (1394 lines) - Complete, consolidated, enterprise-ready

### Key Components

```
AXIOM.py Structure:
├── SkinMetadata (dataclass)
├── SkinDatabase (SQLite with context manager)
├── AdaptiveRateLimiter (smart throttling)
├── SecureExtractor (path traversal protection)
├── AsyncDownloader (aiohttp with progress)
├── MetadataExtractor (robust fallbacks)
├── AsyncScraper (discovery & pagination)
├── AXIOMBranding (startup animations)
└── EnhancedAXIOMScraper (orchestrator)
```

### Database Schema

```sql
CREATE TABLE skins (
    url TEXT PRIMARY KEY,
    title TEXT NOT NULL,
    category TEXT NOT NULL,
    -- ... 20+ metadata fields
    download_status TEXT DEFAULT 'pending',
    local_path TEXT,
    extracted_path TEXT,
    file_hash TEXT
);

-- Indexes for performance
CREATE INDEX idx_category ON skins(category);
CREATE INDEX idx_status ON skins(download_status);
CREATE INDEX idx_download_url ON skins(download_url);
```

## 🔧 Usage Examples

### Basic Scraping
```bash
# Default settings (5 workers, 1s delay)
python AXIOM.py

# Resume interrupted session
python AXIOM.py  # Auto-detects existing database
```

### Performance Tuning
```bash
# Fast mode (home/office network)
python AXIOM.py --workers 10 --delay 0.5 --batch-size 200

# Server-friendly mode
python AXIOM.py --workers 3 --delay 2.0 --batch-size 50

# Maximum throughput (risk of rate limiting)
python AXIOM.py --workers 15 --delay 0.3 --batch-size 300
```

### Database Management
```bash
# View statistics
python axiom_utilities.py stats

# Export to JSON
python axiom_utilities.py export --format json --output skins_backup.json

# Search for skins
python axiom_utilities.py search "weather" --field title

# Reset failed downloads
python axiom_utilities.py reset --status download_failed

# Create backup
python axiom_utilities.py backup

# Compact database
python axiom_utilities.py vacuum
```

## 📊 Output Structure

```
scraped_data/
├── skins.db                      # SQLite database ⭐
├── complete_collection.json      # Full JSON export
├── complete_collection.csv       # CSV for Excel
├── scraping_summary.txt          # Human-readable summary
├── axiom_scraper.log            # Detailed log
├── downloads/                    # Downloaded packages
│   ├── Suites/
│   │   ├── MySuite_v1.0.rmskin
│   │   └── AnotherSuite.zip
│   └── Clocks/
└── extracted_skins/             # Extracted contents
    ├── Suites/
    │   ├── MySuite/
    │   └── AnotherSuite/
    └── Clocks/
```

## 🔒 Security Features

### Path Traversal Protection
- Validates all extraction paths before writing
- Prevents `../../` attacks
- Isolates each skin to its directory

### Size Limits
- Per file: 500 MB maximum
- Per archive: 2 GB maximum
- Prevents zip bombs and disk exhaustion

### Safe Extraction
- Extracts to temporary directory first
- Atomically moves to final location
- Prevents corruption from interruptions

## 🎨 Startup Animations

AXIOM includes optional pygame/numpy animations:

**Purpose**:
- Initialization buffer (gives services time to start)
- Professional branding
- Visual feedback during startup

**Features**:
- 2-second animated progress arc
- Smooth fade-in effects
- Graceful fallback to ASCII if pygame unavailable

**Skip Animation**: Press SPACE or ESC during startup

## 📈 Performance

Typical performance on modern hardware:

| Metric | Value |
|--------|-------|
| Discovery Speed | 100-200 skins/minute |
| Download Speed | 5-15 MB/s (network dependent) |
| Extraction Speed | 20-50 archives/minute |
| Memory Usage | 200-500 MB |
| CPU Usage | 10-30% (multi-threaded) |

**Complete Collection Estimates**:
- 1,000 skins: 1-2 hours
- 5,000 skins: 4-8 hours
- 10,000 skins: 8-16 hours

## 🐛 Troubleshooting

### Common Issues

**"Python not found"**
```bash
# Install Python 3.8+ and ensure it's in PATH
python --version
```

**"Failed to install dependencies"**
```bash
# Update pip
python -m pip install --upgrade pip

# Install manually
pip install aiohttp beautifulsoup4 lxml rarfile py7zr
```

**"Rate limited"**
```bash
# Increase delay between requests
python AXIOM.py --delay 2.0
```

**"Database locked"**
```bash
# Ensure no other AXIOM processes running
# Close any database browsers (DB Browser for SQLite)
```

### Log Files
1. `scraped_data/axiom_scraper.log` - Scraper log
2. `scraped_data/scraping_summary.txt` - Summary report

## 📝 Configuration

Edit `rainmeterui_categories.json` to customize categories:

```json
{
  "rainmeterui_categories": {
    "primary_skin_categories": [
      {
        "name": "Suites",
        "url": "https://rainmeterui.com/category/suites/"
      }
    ],
    "additional_categories": [
      {
        "name": "Clocks",
        "url": "https://rainmeterui.com/category/clocks/"
      }
    ]
  }
}
```

## 🔄 Recent Changes (v2.0)

### October 2025 - Consolidation Release

**Major Changes**:
- ✅ Merged 3 files into single `AXIOM.py`
- ✅ Removed 600+ lines of redundant code
- ✅ Preserved pygame/numpy animations for branding
- ✅ Enhanced metadata extraction with better fallbacks
- ✅ Improved URL validation and filtering

**What Was Removed**:
- Old `AXIOM.py` (1740 lines) - Outdated architecture
- `axiom_enhanced.py` (737 lines) - Incomplete implementation
- `axiom_scraper_core.py` - Now integrated

**What Was Kept**:
- All functionality from v2.0 design
- Startup animations (initialization timing + branding)
- Complete test suite
- All documentation

**Benefits**:
- Single file to maintain
- No duplicate code
- Faster startup (less import overhead)
- Easier deployment

## 📚 Additional Resources

- **Complete Documentation**: `AXIOM v2.0 - Complete Documentation.pdf`
- **Quick Reference**: `AXIOM Quick Reference & Cheat Sheet.pdf`
- **Markdown Docs**: `axiom_documentation.md`, `axiom_quick_reference.md`
- **Test Suite**: `axiom_tests.py`

## 🤝 Contributing

Improvements welcome! Focus areas:
- Additional archive formats (e.g., .tar.gz)
- More metadata extraction patterns
- Better error recovery
- Performance optimizations

## 📜 License

This software is provided as-is for personal and educational use.
Respect RainmeterUI's terms of service and rate limits.

---

**Version**: 2.0.0  
**Last Updated**: October 2025  
**Status**: Production Ready  
**Maintainer**: RainmeterManager Project

---

*AXIOM - Making Rainmeter skin collection effortless*
