#!/usr/bin/env python3
"""
🤖 AXIOM v2.0 - Automated eXtraction Index Operator 🤖

Enterprise-grade Rainmeter skin scraper with:
- SQLite database storage
- Async downloads with aiohttp
- Adaptive rate limiting
- Security features (path traversal protection)
- Resume capability
- Branding with startup animations

Copyright (c) 2024 - Production Ready
"""

import json
import time
import aiohttp
import asyncio
from bs4 import BeautifulSoup
from urllib.parse import urljoin, urlparse, unquote
import logging
from typing import Set, List, Dict, Optional, Tuple
import csv
from pathlib import Path
import re
from dataclasses import dataclass, asdict
import sys
import os
import zipfile
import rarfile
import py7zr
import shutil
import hashlib
from datetime import datetime
import sqlite3
from contextlib import contextmanager
import signal

# Animation imports (optional)
try:
    import pygame
    import numpy as np
    PYGAME_AVAILABLE = True
except ImportError:
    PYGAME_AVAILABLE = False


# ============================================================================
# DATA MODELS
# ============================================================================

@dataclass
class SkinMetadata:
    """Complete metadata for a Rainmeter skin"""
    url: str
    title: str
    category: str
    category_url: str
    page_number: int = 1
    author: str = ""
    description: str = ""
    download_url: str = ""
    download_filename: str = ""
    file_size: str = ""
    downloads_count: str = ""
    rating: str = ""
    tags: str = ""  # JSON string for list
    screenshots: str = ""  # JSON string for list
    created_date: str = ""
    updated_date: str = ""
    version: str = ""
    compatibility: str = ""
    scraped_at: str = ""
    download_status: str = "pending"
    local_path: str = ""
    extracted_path: str = ""
    file_hash: str = ""
    
    def __post_init__(self):
        if not self.scraped_at:
            self.scraped_at = datetime.now().isoformat()
    
    def to_dict(self):
        """Convert to dict with proper list handling"""
        d = asdict(self)
        for field in ['tags', 'screenshots']:
            if isinstance(d[field], str) and d[field]:
                try:
                    d[field] = json.loads(d[field])
                except:
                    d[field] = []
        return d


# ============================================================================
# DATABASE
# ============================================================================

class SkinDatabase:
    """SQLite database for efficient skin storage"""
    
    def __init__(self, db_path: Path):
        self.db_path = db_path
        self.init_database()
    
    @contextmanager
    def get_connection(self):
        """Context manager for database connections"""
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        try:
            yield conn
            conn.commit()
        except Exception:
            conn.rollback()
            raise
        finally:
            conn.close()
    
    def init_database(self):
        """Initialize database schema"""
        with self.get_connection() as conn:
            conn.execute("""
                CREATE TABLE IF NOT EXISTS skins (
                    url TEXT PRIMARY KEY,
                    title TEXT NOT NULL,
                    category TEXT NOT NULL,
                    category_url TEXT,
                    page_number INTEGER DEFAULT 1,
                    author TEXT,
                    description TEXT,
                    download_url TEXT,
                    download_filename TEXT,
                    file_size TEXT,
                    downloads_count TEXT,
                    rating TEXT,
                    tags TEXT,
                    screenshots TEXT,
                    created_date TEXT,
                    updated_date TEXT,
                    version TEXT,
                    compatibility TEXT,
                    scraped_at TEXT,
                    download_status TEXT DEFAULT 'pending',
                    local_path TEXT,
                    extracted_path TEXT,
                    file_hash TEXT
                )
            """)
            
            # Create indexes
            conn.execute("CREATE INDEX IF NOT EXISTS idx_category ON skins(category)")
            conn.execute("CREATE INDEX IF NOT EXISTS idx_status ON skins(download_status)")
            conn.execute("CREATE INDEX IF NOT EXISTS idx_download_url ON skins(download_url)")
    
    def save_skin(self, skin: SkinMetadata) -> bool:
        """Save or update skin metadata"""
        try:
            skin_dict = asdict(skin)
            for field in ['tags', 'screenshots']:
                if isinstance(skin_dict[field], list):
                    skin_dict[field] = json.dumps(skin_dict[field])
            
            with self.get_connection() as conn:
                conn.execute("""
                    INSERT OR REPLACE INTO skins VALUES (
                        :url, :title, :category, :category_url, :page_number,
                        :author, :description, :download_url, :download_filename,
                        :file_size, :downloads_count, :rating, :tags, :screenshots,
                        :created_date, :updated_date, :version, :compatibility,
                        :scraped_at, :download_status, :local_path, :extracted_path,
                        :file_hash
                    )
                """, skin_dict)
            return True
        except Exception as e:
            logging.error(f"Failed to save skin {skin.url}: {e}")
            return False
    
    def get_skin(self, url: str) -> Optional[SkinMetadata]:
        """Retrieve skin by URL"""
        with self.get_connection() as conn:
            cursor = conn.execute("SELECT * FROM skins WHERE url = ?", (url,))
            row = cursor.fetchone()
            if row:
                return SkinMetadata(**dict(row))
        return None
    
    def get_pending_downloads(self, limit: int = None) -> List[SkinMetadata]:
        """Get skins pending download"""
        query = "SELECT * FROM skins WHERE download_status = 'pending' AND download_url != ''"
        if limit:
            query += f" LIMIT {limit}"
        
        with self.get_connection() as conn:
            cursor = conn.execute(query)
            return [SkinMetadata(**dict(row)) for row in cursor.fetchall()]
    
    def update_download_status(self, url: str, status: str, **kwargs):
        """Update download status and related fields"""
        fields = ["download_status = ?"]
        values = [status]
        
        for key, value in kwargs.items():
            fields.append(f"{key} = ?")
            values.append(value)
        
        values.append(url)
        
        with self.get_connection() as conn:
            conn.execute(f"""
                UPDATE skins SET {', '.join(fields)}
                WHERE url = ?
            """, values)
    
    def get_statistics(self) -> Dict:
        """Get scraping statistics"""
        with self.get_connection() as conn:
            stats = {}
            
            cursor = conn.execute("SELECT COUNT(*) FROM skins")
            stats['total_skins'] = cursor.fetchone()[0]
            
            cursor = conn.execute("""
                SELECT download_status, COUNT(*) 
                FROM skins 
                GROUP BY download_status
            """)
            stats['by_status'] = dict(cursor.fetchall())
            
            cursor = conn.execute("""
                SELECT category, COUNT(*) 
                FROM skins 
                GROUP BY category
            """)
            stats['by_category'] = dict(cursor.fetchall())
            
            return stats
    
    def export_to_json(self, output_file: Path):
        """Export all skins to JSON"""
        with self.get_connection() as conn:
            cursor = conn.execute("SELECT * FROM skins")
            skins = [SkinMetadata(**dict(row)).to_dict() for row in cursor.fetchall()]
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(skins, f, indent=2, ensure_ascii=False)
    
    def export_to_csv(self, output_file: Path):
        """Export all skins to CSV"""
        with self.get_connection() as conn:
            cursor = conn.execute("SELECT * FROM skins")
            rows = cursor.fetchall()
            
            if rows:
                with open(output_file, 'w', newline='', encoding='utf-8') as f:
                    writer = csv.DictWriter(f, fieldnames=rows[0].keys())
                    writer.writeheader()
                    for row in rows:
                        writer.writerow(dict(row))


# ============================================================================
# RATE LIMITING
# ============================================================================

class AdaptiveRateLimiter:
    """Smart rate limiting that adapts to server responses"""
    
    def __init__(self, initial_delay: float = 1.0, min_delay: float = 0.5, max_delay: float = 10.0):
        self.delay = initial_delay
        self.min_delay = min_delay
        self.max_delay = max_delay
        self.success_count = 0
        self.consecutive_failures = 0
        self.last_request_time = 0
    
    async def wait(self):
        """Wait appropriate time before next request"""
        if self.last_request_time:
            elapsed = time.time() - self.last_request_time
            wait_time = max(0, self.delay - elapsed)
            if wait_time > 0:
                await asyncio.sleep(wait_time)
        
        self.last_request_time = time.time()
    
    def on_success(self):
        """Called after successful request"""
        self.success_count += 1
        self.consecutive_failures = 0
        
        if self.success_count > 10:
            self.delay = max(self.min_delay, self.delay * 0.95)
            self.success_count = 0
    
    def on_rate_limit(self):
        """Called when rate limited (429)"""
        self.delay = min(self.max_delay, self.delay * 2.0)
        self.success_count = 0
        self.consecutive_failures += 1
    
    def on_error(self):
        """Called on other errors"""
        self.consecutive_failures += 1
        if self.consecutive_failures > 3:
            self.delay = min(self.max_delay, self.delay * 1.5)
    
    def get_current_delay(self) -> float:
        """Get current delay value"""
        return self.delay


# ============================================================================
# SECURE EXTRACTION
# ============================================================================

class SecureExtractor:
    """Secure archive extraction with path traversal protection"""
    
    MAX_EXTRACT_SIZE = 2 * 1024 * 1024 * 1024  # 2GB per archive
    MAX_FILE_SIZE = 500 * 1024 * 1024  # 500MB per file
    
    def __init__(self, logger):
        self.logger = logger
    
    def is_safe_path(self, base_path: Path, target_path: Path) -> bool:
        """Check if extraction path is safe"""
        try:
            base_resolved = base_path.resolve()
            target_resolved = target_path.resolve()
            return target_resolved.is_relative_to(base_resolved)
        except (ValueError, OSError):
            return False
    
    def extract_zip(self, archive_path: Path, extract_dir: Path) -> Tuple[bool, str]:
        """Securely extract ZIP file"""
        try:
            temp_dir = extract_dir.with_name(extract_dir.name + '.tmp')
            temp_dir.mkdir(parents=True, exist_ok=True)
            
            try:
                with zipfile.ZipFile(archive_path, 'r') as zip_ref:
                    total_size = 0
                    
                    for member in zip_ref.namelist():
                        target = temp_dir / member
                        if not self.is_safe_path(temp_dir, target):
                            raise ValueError(f"Unsafe path detected: {member}")
                        
                        file_info = zip_ref.getinfo(member)
                        if file_info.file_size > self.MAX_FILE_SIZE:
                            self.logger.warning(f"Skipping large file: {member}")
                            continue
                        
                        total_size += file_info.file_size
                        if total_size > self.MAX_EXTRACT_SIZE:
                            raise ValueError(f"Archive too large: {total_size} bytes")
                        
                        zip_ref.extract(member, temp_dir)
                
                if extract_dir.exists():
                    shutil.rmtree(extract_dir)
                shutil.move(temp_dir, extract_dir)
                
                self.logger.info(f"Successfully extracted ZIP: {archive_path.name}")
                return True, str(extract_dir)
                
            finally:
                if temp_dir.exists():
                    shutil.rmtree(temp_dir)
                    
        except zipfile.BadZipFile:
            self.logger.error(f"Corrupted ZIP file: {archive_path}")
            return False, ""
        except Exception as e:
            self.logger.error(f"ZIP extraction error: {e}")
            return False, ""
    
    def extract_rar(self, archive_path: Path, extract_dir: Path) -> Tuple[bool, str]:
        """Securely extract RAR file"""
        try:
            temp_dir = extract_dir.with_name(extract_dir.name + '.tmp')
            temp_dir.mkdir(parents=True, exist_ok=True)
            
            try:
                with rarfile.RarFile(archive_path, 'r') as rar_ref:
                    total_size = 0
                    
                    for member in rar_ref.namelist():
                        target = temp_dir / member
                        if not self.is_safe_path(temp_dir, target):
                            raise ValueError(f"Unsafe path detected: {member}")
                        
                        file_info = rar_ref.getinfo(member)
                        if file_info.file_size > self.MAX_FILE_SIZE:
                            self.logger.warning(f"Skipping large file: {member}")
                            continue
                        
                        total_size += file_info.file_size
                        if total_size > self.MAX_EXTRACT_SIZE:
                            raise ValueError(f"Archive too large: {total_size} bytes")
                    
                    rar_ref.extractall(temp_dir)
                
                if extract_dir.exists():
                    shutil.rmtree(extract_dir)
                shutil.move(temp_dir, extract_dir)
                
                self.logger.info(f"Successfully extracted RAR: {archive_path.name}")
                return True, str(extract_dir)
                
            finally:
                if temp_dir.exists():
                    shutil.rmtree(temp_dir)
                    
        except rarfile.BadRarFile:
            self.logger.error(f"Corrupted RAR file: {archive_path}")
            return False, ""
        except Exception as e:
            self.logger.error(f"RAR extraction error: {e}")
            return False, ""
    
    def extract_7z(self, archive_path: Path, extract_dir: Path) -> Tuple[bool, str]:
        """Securely extract 7Z file"""
        try:
            temp_dir = extract_dir.with_name(extract_dir.name + '.tmp')
            temp_dir.mkdir(parents=True, exist_ok=True)
            
            try:
                with py7zr.SevenZipFile(archive_path, 'r') as archive:
                    archive.extractall(temp_dir)
                
                if extract_dir.exists():
                    shutil.rmtree(extract_dir)
                shutil.move(temp_dir, extract_dir)
                
                self.logger.info(f"Successfully extracted 7Z: {archive_path.name}")
                return True, str(extract_dir)
                
            finally:
                if temp_dir.exists():
                    shutil.rmtree(temp_dir)
                    
        except Exception as e:
            self.logger.error(f"7Z extraction error: {e}")
            return False, ""


# ============================================================================
# ASYNC DOWNLOADER
# ============================================================================

class AsyncDownloader:
    """Async file downloader with progress tracking"""
    
    MAX_DOWNLOAD_SIZE = 500 * 1024 * 1024  # 500MB
    CHUNK_SIZE = 8192
    
    def __init__(self, download_dir: Path, logger, rate_limiter: AdaptiveRateLimiter):
        self.download_dir = download_dir
        self.logger = logger
        self.rate_limiter = rate_limiter
        self.session = None
    
    async def init_session(self):
        """Initialize aiohttp session"""
        timeout = aiohttp.ClientTimeout(total=300, connect=30)
        self.session = aiohttp.ClientSession(
            timeout=timeout,
            headers={
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
            }
        )
    
    async def close_session(self):
        """Close aiohttp session"""
        if self.session:
            await self.session.close()
    
    def get_filename_from_url(self, url: str, content_disposition: str = None) -> str:
        """Extract filename from URL or headers"""
        if content_disposition:
            cd_match = re.search(r'filename[*]?=(?:UTF-8\'\')?["\']?([^"\';]+)["\']?', content_disposition)
            if cd_match:
                filename = unquote(cd_match.group(1))
                if filename and not filename.startswith('.'):
                    return filename
        
        parsed = urlparse(url)
        filename = Path(parsed.path).name
        
        if filename and '.' in filename:
            return unquote(filename)
        
        url_hash = hashlib.md5(url.encode()).hexdigest()[:8]
        return f"skin_{url_hash}.zip"
    
    async def validate_download_url(self, url: str) -> bool:
        """Validate URL with HEAD request"""
        try:
            await self.rate_limiter.wait()
            
            async with self.session.head(url, allow_redirects=True) as response:
                if response.status == 200:
                    content_type = response.headers.get('content-type', '').lower()
                    content_length = response.headers.get('content-length', 0)
                    
                    valid_types = ['zip', 'rar', '7z', 'octet-stream', 'application/x-']
                    if not any(t in content_type for t in valid_types):
                        self.logger.warning(f"Invalid content type: {content_type}")
                        return False
                    
                    if int(content_length) > self.MAX_DOWNLOAD_SIZE:
                        self.logger.warning(f"File too large: {content_length} bytes")
                        return False
                    
                    return True
                else:
                    return False
                    
        except Exception as e:
            self.logger.debug(f"HEAD request failed for {url}: {e}")
            return True  # Allow download attempt anyway
    
    async def download_file(self, url: str, category: str, skin_name: str) -> Tuple[bool, str, str]:
        """Download file asynchronously"""
        try:
            await self.rate_limiter.wait()
            
            self.logger.info(f"Downloading: {url}")
            
            category_dir = self.download_dir / self.sanitize_filename(category)
            category_dir.mkdir(exist_ok=True)
            
            async with self.session.get(url) as response:
                if response.status == 429:
                    self.rate_limiter.on_rate_limit()
                    self.logger.warning("Rate limited, backing off...")
                    return False, "", ""
                
                response.raise_for_status()
                
                content_disposition = response.headers.get('content-disposition')
                filename = self.get_filename_from_url(url, content_disposition)
                
                if not any(filename.lower().endswith(ext) for ext in ['.zip', '.rar', '.7z', '.rmskin']):
                    content_type = response.headers.get('content-type', '').lower()
                    if 'zip' in content_type:
                        filename += '.zip'
                    elif 'rar' in content_type:
                        filename += '.rar'
                    else:
                        filename += '.zip'
                
                safe_skin_name = self.sanitize_filename(skin_name)
                local_path = category_dir / f"{safe_skin_name}_{filename}"
                
                total_size = int(response.headers.get('content-length', 0))
                if total_size > self.MAX_DOWNLOAD_SIZE:
                    self.logger.error(f"File too large: {total_size} bytes")
                    return False, "", ""
                
                downloaded = 0
                hasher = hashlib.sha256()
                
                with open(local_path, 'wb') as f:
                    async for chunk in response.content.iter_chunked(self.CHUNK_SIZE):
                        if chunk:
                            f.write(chunk)
                            hasher.update(chunk)
                            downloaded += len(chunk)
                
                file_hash = hasher.hexdigest()
                self.rate_limiter.on_success()
                self.logger.info(f"Downloaded: {local_path} ({downloaded} bytes)")
                
                return True, str(local_path), file_hash
                
        except aiohttp.ClientError as e:
            self.rate_limiter.on_error()
            self.logger.error(f"Download failed for {url}: {e}")
            return False, "", ""
        except Exception as e:
            self.rate_limiter.on_error()
            self.logger.error(f"Unexpected error downloading {url}: {e}")
            return False, "", ""
    
    def sanitize_filename(self, filename: str) -> str:
        """Sanitize filename for filesystem"""
        invalid_chars = '<>:"/\\|?*'
        for char in invalid_chars:
            filename = filename.replace(char, '_')
        filename = filename[:100]
        filename = filename.strip('. ')
        return filename if filename else "unnamed"


# ============================================================================
# METADATA EXTRACTION
# ============================================================================

class MetadataExtractor:
    """Extract metadata from skin pages"""
    
    def __init__(self, logger):
        self.logger = logger
    
    def extract_from_soup(self, soup: BeautifulSoup, url: str, category: str, 
                         category_url: str, page_num: int = 1) -> Optional[SkinMetadata]:
        """Extract complete metadata from page"""
        try:
            metadata = SkinMetadata(
                url=url,
                title=self.extract_title(soup),
                category=category,
                category_url=category_url,
                page_number=page_num,
                author=self.extract_author(soup),
                description=self.extract_description(soup),
                download_url=self.extract_download_url(soup, url),
                file_size=self.extract_file_size(soup),
                downloads_count=self.extract_downloads_count(soup),
                rating=self.extract_rating(soup),
                tags=json.dumps(self.extract_tags(soup)),
                screenshots=json.dumps(self.extract_screenshots(soup, url)),
                created_date=self.extract_date(soup),
                version=self.extract_version(soup),
                compatibility=self.extract_compatibility(soup)
            )
            
            return metadata
            
        except Exception as e:
            self.logger.error(f"Failed to extract metadata from {url}: {e}")
            return None
    
    def extract_title(self, soup: BeautifulSoup) -> str:
        """Extract title with multiple fallbacks"""
        selectors = ['h1.entry-title', 'h1.post-title', '.entry-header h1', 'h1', 'title']
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                title = element.get_text(strip=True)
                if title and len(title) > 3:
                    return title
        return "Unknown Skin"
    
    def extract_author(self, soup: BeautifulSoup) -> str:
        """Extract author"""
        selectors = ['.author a', '.post-author a', '.entry-meta a[rel="author"]', '[class*="author"] a']
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                author = element.get_text(strip=True)
                if author:
                    return author
        return "Unknown Author"
    
    def extract_description(self, soup: BeautifulSoup) -> str:
        """Extract description"""
        selectors = ['.entry-content', '.post-content', '.content', 'article .text', '.description']
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                for script in element(["script", "style"]):
                    script.decompose()
                description = element.get_text(strip=True)
                if description and len(description) > 20:
                    return description[:2000]
        return ""
    
    def extract_download_url(self, soup: BeautifulSoup, base_url: str) -> str:
        """Extract download URL with validation"""
        selectors = [
            'a[href*="download"]', 'a[href*=".rmskin"]', 'a[href*=".zip"]',
            'a[href*=".rar"]', '.download-btn', '.download-link', 'a[class*="download"]'
        ]
        for selector in selectors:
            elements = soup.select(selector)
            for element in elements:
                href = element.get('href')
                if href:
                    download_url = urljoin(base_url, href)
                    if self.is_valid_download_url(download_url):
                        return download_url
        return ""
    
    def is_valid_download_url(self, url: str) -> bool:
        """Validate download URL"""
        if not url:
            return False
        parsed = urlparse(url)
        path = parsed.path.lower()
        download_extensions = ['.rmskin', '.zip', '.rar', '.7z']
        if any(path.endswith(ext) for ext in download_extensions):
            return True
        download_indicators = ['download', 'get', 'file']
        if any(indicator in path for indicator in download_indicators):
            return True
        return False
    
    def extract_file_size(self, soup: BeautifulSoup) -> str:
        """Extract file size"""
        size_pattern = re.compile(r'(\d+(?:\.\d+)?)\s*(KB|MB|GB)', re.IGNORECASE)
        text = soup.get_text()
        match = size_pattern.search(text)
        if match:
            return f"{match.group(1)} {match.group(2).upper()}"
        return ""
    
    def extract_downloads_count(self, soup: BeautifulSoup) -> str:
        """Extract download count"""
        patterns = [
            re.compile(r'(\d+)\s*downloads?', re.IGNORECASE),
            re.compile(r'downloaded\s*(\d+)\s*times?', re.IGNORECASE)
        ]
        text = soup.get_text()
        for pattern in patterns:
            match = pattern.search(text)
            if match:
                return match.group(1)
        return ""
    
    def extract_rating(self, soup: BeautifulSoup) -> str:
        """Extract rating"""
        selectors = ['.rating', '.stars', '[class*="rating"]']
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                rating = element.get_text(strip=True)
                if rating:
                    return rating
        return ""
    
    def extract_tags(self, soup: BeautifulSoup) -> List[str]:
        """Extract tags"""
        tags = []
        selectors = ['.tags a', '.tag-links a', '.post-tags a', '[class*="tag"] a']
        for selector in selectors:
            elements = soup.select(selector)
            for element in elements:
                tag = element.get_text(strip=True)
                if tag and tag not in tags:
                    tags.append(tag)
        return tags[:20]  # Limit to 20
    
    def extract_screenshots(self, soup: BeautifulSoup, base_url: str) -> List[str]:
        """Extract screenshot URLs"""
        screenshots = []
        img_elements = soup.select('.entry-content img, .post-content img, article img')
        for img in img_elements:
            src = img.get('src') or img.get('data-src')
            if src:
                full_url = urljoin(base_url, src)
                if self.is_screenshot_url(full_url):
                    screenshots.append(full_url)
        return screenshots[:10]  # Limit to 10
    
    def is_screenshot_url(self, url: str) -> bool:
        """Check if URL is screenshot"""
        if not url:
            return False
        path = urlparse(url).path.lower()
        img_extensions = ['.jpg', '.jpeg', '.png', '.gif', '.webp']
        return any(path.endswith(ext) for ext in img_extensions)
    
    def extract_date(self, soup: BeautifulSoup) -> str:
        """Extract date"""
        selectors = ['.entry-date', '.post-date', 'time', '.date']
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                datetime_attr = element.get('datetime')
                if datetime_attr:
                    return datetime_attr
                date_text = element.get_text(strip=True)
                if date_text:
                    return date_text
        return ""
    
    def extract_version(self, soup: BeautifulSoup) -> str:
        """Extract version"""
        version_pattern = re.compile(r'version\s*:?\s*([0-9.]+)', re.IGNORECASE)
        text = soup.get_text()
        match = version_pattern.search(text)
        if match:
            return match.group(1)
        return ""
    
    def extract_compatibility(self, soup: BeautifulSoup) -> str:
        """Extract Rainmeter compatibility"""
        compat_pattern = re.compile(r'rainmeter\s*([0-9.]+)', re.IGNORECASE)
        text = soup.get_text()
        match = compat_pattern.search(text)
        if match:
            return f"Rainmeter {match.group(1)}"
        return ""


# ============================================================================
# ASYNC SCRAPER
# ============================================================================

class AsyncScraper:
    """Async web scraper for discovery"""
    
    def __init__(self, logger, rate_limiter):
        self.logger = logger
        self.rate_limiter = rate_limiter
        self.session = None
        self.metadata_extractor = MetadataExtractor(logger)
    
    async def init_session(self):
        """Initialize session"""
        timeout = aiohttp.ClientTimeout(total=60, connect=10)
        self.session = aiohttp.ClientSession(
            timeout=timeout,
            headers={
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36',
                'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
                'Accept-Language': 'en-US,en;q=0.9'
            }
        )
    
    async def close_session(self):
        """Close session"""
        if self.session:
            await self.session.close()
    
    async def fetch_page(self, url: str) -> Optional[BeautifulSoup]:
        """Fetch and parse page"""
        try:
            await self.rate_limiter.wait()
            self.logger.info(f"Fetching: {url}")
            
            async with self.session.get(url) as response:
                if response.status == 429:
                    self.rate_limiter.on_rate_limit()
                    self.logger.warning("Rate limited")
                    return None
                
                response.raise_for_status()
                html = await response.text()
                self.rate_limiter.on_success()
                
                return BeautifulSoup(html, 'html.parser')
                
        except Exception as e:
            self.rate_limiter.on_error()
            self.logger.error(f"Failed to fetch {url}: {e}")
            return None
    
    def find_skin_urls(self, soup: BeautifulSoup, category_url: str) -> List[str]:
        """Find skin page URLs"""
        urls = set()
        selectors = [
            'article h2 a', '.post-title a', '.entry-title a',
            'h3 a', '.skin-item a', 'a[href*="rainmeterui.com"]'
        ]
        for selector in selectors:
            elements = soup.select(selector)
            for element in elements:
                href = element.get('href')
                if href:
                    full_url = urljoin(category_url, href)
                    if self.is_skin_page_url(full_url):
                        urls.add(full_url)
        return list(urls)
    
    def is_skin_page_url(self, url: str) -> bool:
        """Validate skin page URL"""
        if not url or not url.startswith('http'):
            return False
        parsed = urlparse(url)
        if 'rainmeterui.com' not in parsed.netloc.lower():
            return False
        path = parsed.path.lower()
        exclude_patterns = ['/tag/', '/category/', '/archive/', '/page/', '/author/', '/date/', '/redirect/', '.xml', '.rss']
        if any(pattern in path for pattern in exclude_patterns):
            return False
        path_parts = path.strip('/').split('/')
        return len(path_parts) >= 1 and not path.endswith('/')
    
    def find_next_page(self, soup: BeautifulSoup, current_url: str) -> Optional[str]:
        """Find next page URL"""
        selectors = ['a.next', 'a[rel="next"]', '.next-page a', '.pagination-next a', '.nav-next a']
        for selector in selectors:
            next_link = soup.select_one(selector)
            if next_link and next_link.get('href'):
                return urljoin(current_url, next_link.get('href'))
        return None
    
    async def scrape_category(self, category: Dict, db) -> int:
        """Scrape entire category"""
        category_name = category.get('name', 'Unknown')
        category_url = category.get('url')
        
        if not category_url:
            return 0
        
        self.logger.info(f"Scraping category: {category_name}")
        
        skins_found = 0
        current_url = category_url
        page_num = 1
        max_pages = 50
        
        while current_url and page_num <= max_pages:
            self.logger.info(f"Page {page_num}: {current_url}")
            
            soup = await self.fetch_page(current_url)
            if not soup:
                break
            
            skin_urls = self.find_skin_urls(soup, category_url)
            self.logger.info(f"Found {len(skin_urls)} skins on page {page_num}")
            
            for skin_url in skin_urls:
                if db.get_skin(skin_url):
                    self.logger.debug(f"Skipping existing: {skin_url}")
                    continue
                
                skin_soup = await self.fetch_page(skin_url)
                if skin_soup:
                    metadata = self.metadata_extractor.extract_from_soup(
                        skin_soup, skin_url, category_name, category_url, page_num
                    )
                    if metadata:
                        db.save_skin(metadata)
                        skins_found += 1
            
            next_url = self.find_next_page(soup, current_url)
            if next_url and next_url != current_url:
                current_url = next_url
                page_num += 1
            else:
                break
        
        self.logger.info(f"Category {category_name}: {skins_found} new skins")
        return skins_found


# ============================================================================
# BRANDING SYSTEM
# ============================================================================

class AXIOMBranding:
    """AXIOM branding with startup animations"""
    
    def __init__(self, enable_animation=True, enable_sound=True):
        self.enable_animation = enable_animation and PYGAME_AVAILABLE
        self.enable_sound = enable_sound and PYGAME_AVAILABLE
        self.screen = None
        self.clock = None
        
        if self.enable_animation or self.enable_sound:
            self.init_pygame()
    
    def init_pygame(self):
        """Initialize pygame"""
        try:
            pygame.init()
            if self.enable_sound:
                pygame.mixer.init(frequency=22050, size=-16, channels=2, buffer=512)
            
            if self.enable_animation:
                self.screen_width = 1200
                self.screen_height = 800
                self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
                pygame.display.set_caption("🤖 AXIOM - Automated eXtraction Index Operator")
                self.clock = pygame.time.Clock()
                
        except Exception as e:
            print(f"⚠️  Could not initialize pygame: {e}")
            self.enable_animation = False
            self.enable_sound = False
    
    def show_startup_sequence(self):
        """Show AXIOM startup"""
        print("\n" + "=" * 70)
        print("🤖 AXIOM v2.0 STARTUP - Enhanced Edition")
        print("=" * 70)
        
        if self.enable_animation:
            print("🎬 Launching visual interface...")
            self.create_startup_animation()
        else:
            self.ascii_startup_animation()
        
        print("\n✅ AXIOM fully operational!")
        print("=" * 70 + "\n")
    
    def create_startup_animation(self):
        """Simple startup animation"""
        DEEP_BLACK = (8, 12, 20)
        AXIOM_CYAN = (0, 255, 255)
        ELECTRIC_BLUE = (100, 200, 255)
        
        animation_time = 0
        total_duration = 2.0
        
        while animation_time < total_duration:
            dt = self.clock.tick(60) / 1000.0
            animation_time += dt
            
            for event in pygame.event.get():
                if event.type == pygame.QUIT or \
                   (event.type == pygame.KEYDOWN and event.key in [pygame.K_SPACE, pygame.K_ESCAPE]):
                    pygame.quit()
                    return
            
            self.screen.fill(DEEP_BLACK)
            
            # Progress circle
            progress = animation_time / total_duration
            center_x = self.screen_width // 2
            center_y = self.screen_height // 2
            radius = 100
            
            # Draw progress arc
            angle = int(360 * progress)
            if angle > 0:
                rect = pygame.Rect(center_x - radius, center_y - radius, radius * 2, radius * 2)
                pygame.draw.arc(self.screen, AXIOM_CYAN, rect, 0, np.radians(angle), 5)
            
            # AXIOM text
            if animation_time > 0.5:
                try:
                    font = pygame.font.Font(None, 72)
                    text = font.render("AXIOM v2.0", True, ELECTRIC_BLUE)
                    text_rect = text.get_rect(center=(center_x, center_y + 150))
                    self.screen.blit(text, text_rect)
                except:
                    pass
            
            pygame.display.flip()
        
        pygame.quit()
    
    def ascii_startup_animation(self):
        """ASCII fallback"""
        frames = [
            "    [        ] Initializing...",
            "    [▓▓      ] Loading modules...",
            "    [▓▓▓▓    ] Connecting...",
            "    [▓▓▓▓▓▓  ] Starting services...",
            "    [▓▓▓▓▓▓▓▓] Ready!",
        ]
        
        for frame in frames:
            print(f"\r{frame}", end='', flush=True)
            time.sleep(0.4)
        print()


# ============================================================================
# MAIN ORCHESTRATOR
# ============================================================================

class EnhancedAXIOMScraper:
    """Main orchestrator for AXIOM scraper"""
    
    def __init__(self, config_file: str, output_dir: str, delay: float = 1.0,
                 max_workers: int = 5, batch_size: int = 100):
        self.config_file = config_file
        self.output_dir = Path(output_dir)
        self.delay = delay
        self.max_workers = max_workers
        self.batch_size = batch_size
        
        # Setup directories
        self.output_dir.mkdir(exist_ok=True)
        self.downloads_dir = self.output_dir / "downloads"
        self.extracted_dir = self.output_dir / "extracted_skins"
        self.downloads_dir.mkdir(exist_ok=True)
        self.extracted_dir.mkdir(exist_ok=True)
        
        # Setup logging
        self.setup_logging()
        
        # Load config
        self.config = self.load_config()
        
        # Initialize components
        self.db = SkinDatabase(self.output_dir / "skins.db")
        self.rate_limiter = AdaptiveRateLimiter(initial_delay=delay)
        self.scraper = AsyncScraper(self.logger, self.rate_limiter)
        self.downloader = AsyncDownloader(self.downloads_dir, self.logger, self.rate_limiter)
        self.extractor = SecureExtractor(self.logger)
        
        # Statistics
        self.stats = {
            'categories_processed': 0,
            'skins_discovered': 0,
            'skins_downloaded': 0,
            'skins_extracted': 0,
            'download_failures': 0,
            'extraction_failures': 0
        }
        
        # Graceful shutdown
        self.shutdown_requested = False
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
    
    def setup_logging(self):
        """Setup logging"""
        log_file = self.output_dir / "axiom_scraper.log"
        
        formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        
        file_handler = logging.FileHandler(log_file, encoding='utf-8')
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(formatter)
        
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(formatter)
        
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(logging.DEBUG)
        self.logger.addHandler(file_handler)
        self.logger.addHandler(console_handler)
    
    def load_config(self) -> Dict:
        """Load configuration"""
        try:
            with open(self.config_file, 'r', encoding='utf-8') as f:
                config = json.load(f)
            return config.get('rainmeterui_categories', config)
        except Exception as e:
            self.logger.error(f"Failed to load config: {e}")
            raise
    
    def signal_handler(self, signum, frame):
        """Handle shutdown signals"""
        self.logger.info("Shutdown requested, finishing current tasks...")
        self.shutdown_requested = True
    
    async def run_discovery_phase(self):
        """Phase 1: Discover and scrape metadata"""
        self.logger.info("=" * 70)
        self.logger.info("PHASE 1: DISCOVERY - Scraping metadata")
        self.logger.info("=" * 70)
        
        categories = []
        if 'primary_skin_categories' in self.config:
            categories.extend(self.config['primary_skin_categories'])
        if 'additional_categories' in self.config:
            categories.extend(self.config['additional_categories'])
        
        await self.scraper.init_session()
        
        try:
            for idx, category in enumerate(categories, 1):
                if self.shutdown_requested:
                    break
                
                self.logger.info(f"Category {idx}/{len(categories)}: {category.get('name')}")
                skins_found = await self.scraper.scrape_category(category, self.db)
                self.stats['skins_discovered'] += skins_found
                self.stats['categories_processed'] += 1
        
        finally:
            await self.scraper.close_session()
        
        self.logger.info(f"Discovery complete: {self.stats['skins_discovered']} skins found")
    
    async def run_download_phase(self):
        """Phase 2: Download skin packages"""
        self.logger.info("=" * 70)
        self.logger.info("PHASE 2: DOWNLOAD - Fetching skin packages")
        self.logger.info("=" * 70)
        
        await self.downloader.init_session()
        
        try:
            batch_num = 0
            while not self.shutdown_requested:
                pending = self.db.get_pending_downloads(limit=self.batch_size)
                if not pending:
                    break
                
                batch_num += 1
                self.logger.info(f"Batch {batch_num}: {len(pending)} skins")
                
                validated_skins = []
                for skin in pending:
                    if await self.downloader.validate_download_url(skin.download_url):
                        validated_skins.append(skin)
                    else:
                        self.db.update_download_status(skin.url, 'invalid_url')
                
                tasks = []
                for skin in validated_skins:
                    task = self.download_skin(skin)
                    tasks.append(task)
                
                results = await asyncio.gather(*tasks, return_exceptions=True)
                
                for skin, result in zip(validated_skins, results):
                    if isinstance(result, Exception):
                        self.logger.error(f"Download error for {skin.title}: {result}")
                        self.db.update_download_status(skin.url, 'download_failed')
                        self.stats['download_failures'] += 1
        
        finally:
            await self.downloader.close_session()
        
        self.logger.info(f"Downloads complete: {self.stats['skins_downloaded']} successful")
    
    async def download_skin(self, skin):
        """Download single skin"""
        success, local_path, file_hash = await self.downloader.download_file(
            skin.download_url, skin.category, skin.title
        )
        
        if success:
            self.db.update_download_status(
                skin.url, 'downloaded',
                local_path=local_path,
                file_hash=file_hash
            )
            self.stats['skins_downloaded'] += 1
            return True
        else:
            self.db.update_download_status(skin.url, 'download_failed')
            self.stats['download_failures'] += 1
            return False
    
    def run_extraction_phase(self):
        """Phase 3: Extract archives"""
        self.logger.info("=" * 70)
        self.logger.info("PHASE 3: EXTRACTION - Unpacking archives")
        self.logger.info("=" * 70)
        
        with self.db.get_connection() as conn:
            cursor = conn.execute(
                "SELECT * FROM skins WHERE download_status = 'downloaded'"
            )
            skins = [dict(row) for row in cursor.fetchall()]
        
        for skin_data in skins:
            if self.shutdown_requested:
                break
            
            skin = SkinMetadata(**skin_data)
            
            if not skin.local_path or not Path(skin.local_path).exists():
                continue
            
            archive_path = Path(skin.local_path)
            file_ext = archive_path.suffix.lower()
            
            success = False
            extract_path = ""
            
            category_dir = self.extracted_dir / self.sanitize_filename(skin.category)
            skin_dir = category_dir / self.sanitize_filename(skin.title)
            
            if file_ext in ['.zip', '.rmskin']:
                success, extract_path = self.extractor.extract_zip(archive_path, skin_dir)
            elif file_ext == '.rar':
                success, extract_path = self.extractor.extract_rar(archive_path, skin_dir)
            elif file_ext == '.7z':
                success, extract_path = self.extractor.extract_7z(archive_path, skin_dir)
            
            if success:
                self.db.update_download_status(
                    skin.url, 'extracted',
                    extracted_path=extract_path
                )
                self.stats['skins_extracted'] += 1
            else:
                self.db.update_download_status(skin.url, 'extraction_failed')
                self.stats['extraction_failures'] += 1
        
        self.logger.info(f"Extraction complete: {self.stats['skins_extracted']} skins extracted")
    
    def sanitize_filename(self, filename: str) -> str:
        """Sanitize filename"""
        invalid_chars = '<>:"/\\|?*'
        for char in invalid_chars:
            filename = filename.replace(char, '_')
        return filename[:100].strip('. ')
    
    def save_final_reports(self):
        """Save final reports"""
        self.logger.info("Generating final reports...")
        
        self.db.export_to_json(self.output_dir / "complete_collection.json")
        self.db.export_to_csv(self.output_dir / "complete_collection.csv")
        
        db_stats = self.db.get_statistics()
        
        summary_file = self.output_dir / "scraping_summary.txt"
        with open(summary_file, 'w', encoding='utf-8') as f:
            f.write("AXIOM SCRAPING SUMMARY\n")
            f.write("=" * 70 + "\n\n")
            f.write(f"Categories processed: {self.stats['categories_processed']}\n")
            f.write(f"Skins discovered: {self.stats['skins_discovered']}\n")
            f.write(f"Skins downloaded: {self.stats['skins_downloaded']}\n")
            f.write(f"Skins extracted: {self.stats['skins_extracted']}\n")
            f.write(f"Download failures: {self.stats['download_failures']}\n")
            f.write(f"Extraction failures: {self.stats['extraction_failures']}\n\n")
            f.write("Database Statistics:\n")
            f.write(f"Total skins in database: {db_stats['total_skins']}\n\n")
            f.write("Status breakdown:\n")
            for status, count in db_stats.get('by_status', {}).items():
                f.write(f"  {status}: {count}\n")
        
        self.logger.info(f"Reports saved to {self.output_dir}")
    
    async def run(self):
        """Run complete scraping pipeline"""
        branding = AXIOMBranding()
        branding.show_startup_sequence()
        
        try:
            await self.run_discovery_phase()
            
            if not self.shutdown_requested:
                await self.run_download_phase()
            
            if not self.shutdown_requested:
                self.run_extraction_phase()
            
            self.save_final_reports()
            
            self.logger.info("=" * 70)
            self.logger.info("🎉 AXIOM SCRAPING COMPLETE! 🎉")
            self.logger.info("=" * 70)
            
        except Exception as e:
            self.logger.error(f"Fatal error: {e}", exc_info=True)
            raise


# ============================================================================
# MAIN ENTRY POINT
# ============================================================================

def main():
    """Enhanced main function"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="AXIOM Enhanced Scraper v2.0",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python AXIOM_v2.py                                    # Run with defaults
  python AXIOM_v2.py --workers 10 --delay 0.5          # Fast mode
  python AXIOM_v2.py --output my_collection            # Custom output dir
  python AXIOM_v2.py --batch-size 200                  # Larger batches
        """
    )
    
    parser.add_argument('--config', default='rainmeterui_categories.json',
                       help='Configuration file (default: rainmeterui_categories.json)')
    parser.add_argument('--output', default='scraped_data',
                       help='Output directory (default: scraped_data)')
    parser.add_argument('--delay', type=float, default=1.0,
                       help='Request delay in seconds (default: 1.0)')
    parser.add_argument('--workers', type=int, default=5,
                       help='Parallel download workers (default: 5)')
    parser.add_argument('--batch-size', type=int, default=100,
                       help='Download batch size (default: 100)')
    
    args = parser.parse_args()
    
    print("🚀 AXIOM Enhanced Scraper v2.0 Starting...")
    print(f"📁 Output: {args.output}")
    print(f"⚡ Workers: {args.workers}")
    print(f"📦 Batch size: {args.batch_size}")
    print()
    
    scraper = EnhancedAXIOMScraper(
        config_file=args.config,
        output_dir=args.output,
        delay=args.delay,
        max_workers=args.workers,
        batch_size=args.batch_size
    )
    
    asyncio.run(scraper.run())


if __name__ == "__main__":
    main()
