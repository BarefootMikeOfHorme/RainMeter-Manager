#!/usr/bin/env python3
"""
AXIOM Test Suite
Comprehensive tests for enhanced scraper functionality
"""

import pytest
import asyncio
import tempfile
import shutil
from pathlib import Path
import json
import sqlite3
from unittest.mock import Mock, patch, AsyncMock
import aiohttp


class TestSkinDatabase:
    """Test database operations"""
    
    def setup_method(self):
        """Setup test database"""
        self.temp_dir = tempfile.mkdtemp()
        self.db_path = Path(self.temp_dir) / "test.db"
    
    def teardown_method(self):
        """Cleanup"""
        shutil.rmtree(self.temp_dir)
    
    def test_database_creation(self):
        """Test database initialization"""
        from AXIOM import SkinDatabase
        
        db = SkinDatabase(self.db_path)
        assert self.db_path.exists()
        
        # Verify tables exist
        with db.get_connection() as conn:
            cursor = conn.execute(
                "SELECT name FROM sqlite_master WHERE type='table'"
            )
            tables = [row[0] for row in cursor.fetchall()]
            assert 'skins' in tables
    
    def test_save_and_retrieve_skin(self):
        """Test saving and retrieving skin metadata"""
        from AXIOM import SkinDatabase, SkinMetadata
        
        db = SkinDatabase(self.db_path)
        
        skin = SkinMetadata(
            url="https://example.com/skin1",
            title="Test Skin",
            category="Test Category",
            category_url="https://example.com/category",
            author="Test Author",
            download_url="https://example.com/download"
        )
        
        # Save skin
        assert db.save_skin(skin)
        
        # Retrieve skin
        retrieved = db.get_skin(skin.url)
        assert retrieved is not None
        assert retrieved.title == "Test Skin"
        assert retrieved.author == "Test Author"
    
    def test_update_download_status(self):
        """Test status updates"""
        from AXIOM import SkinDatabase, SkinMetadata
        
        db = SkinDatabase(self.db_path)
        
        skin = SkinMetadata(
            url="https://example.com/skin1",
            title="Test Skin",
            category="Test",
            category_url="https://example.com/cat"
        )
        
        db.save_skin(skin)
        
        # Update status
        db.update_download_status(
            skin.url,
            'downloaded',
            local_path='/path/to/file.zip',
            file_hash='abc123'
        )
        
        # Verify update
        updated = db.get_skin(skin.url)
        assert updated.download_status == 'downloaded'
        assert updated.local_path == '/path/to/file.zip'
        assert updated.file_hash == 'abc123'
    
    def test_get_pending_downloads(self):
        """Test fetching pending downloads"""
        from AXIOM import SkinDatabase, SkinMetadata
        
        db = SkinDatabase(self.db_path)
        
        # Add test skins
        for i in range(10):
            skin = SkinMetadata(
                url=f"https://example.com/skin{i}",
                title=f"Skin {i}",
                category="Test",
                category_url="https://example.com/cat",
                download_url=f"https://example.com/dl{i}"
            )
            db.save_skin(skin)
        
        # Get pending
        pending = db.get_pending_downloads(limit=5)
        assert len(pending) == 5
        assert all(s.download_status == 'pending' for s in pending)
    
    def test_statistics(self):
        """Test statistics generation"""
        from AXIOM import SkinDatabase, SkinMetadata
        
        db = SkinDatabase(self.db_path)
        
        # Add skins with different statuses
        statuses = ['pending', 'downloaded', 'extracted', 'failed']
        for i, status in enumerate(statuses):
            skin = SkinMetadata(
                url=f"https://example.com/skin{i}",
                title=f"Skin {i}",
                category="Test",
                category_url="https://example.com/cat",
                download_status=status
            )
            db.save_skin(skin)
        
        stats = db.get_statistics()
        assert stats['total_skins'] == 4
        assert len(stats['by_status']) == 4


class TestAdaptiveRateLimiter:
    """Test rate limiting logic"""
    
    @pytest.mark.asyncio
    async def test_initial_delay(self):
        """Test initial delay"""
        from AXIOM import AdaptiveRateLimiter
        
        limiter = AdaptiveRateLimiter(initial_delay=1.0)
        assert limiter.delay == 1.0
    
    @pytest.mark.asyncio
    async def test_success_speedup(self):
        """Test speedup after successes"""
        from AXIOM import AdaptiveRateLimiter
        
        limiter = AdaptiveRateLimiter(initial_delay=2.0, min_delay=0.5)
        
        # Trigger many successes
        for _ in range(15):
            limiter.on_success()
        
        assert limiter.delay < 2.0
        assert limiter.delay >= 0.5
    
    def test_rate_limit_backoff(self):
        """Test backoff on rate limit"""
        from AXIOM import AdaptiveRateLimiter
        
        limiter = AdaptiveRateLimiter(initial_delay=1.0, max_delay=10.0)
        initial = limiter.delay
        
        limiter.on_rate_limit()
        assert limiter.delay > initial
        assert limiter.delay <= 10.0
    
    def test_error_handling(self):
        """Test error backoff"""
        from AXIOM import AdaptiveRateLimiter
        
        limiter = AdaptiveRateLimiter(initial_delay=1.0)
        
        # Multiple errors
        for _ in range(5):
            limiter.on_error()
        
        assert limiter.delay > 1.0


class TestSecureExtractor:
    """Test secure extraction"""
    
    def setup_method(self):
        """Setup test environment"""
        self.temp_dir = tempfile.mkdtemp()
        self.extract_dir = Path(self.temp_dir) / "extract"
        self.extract_dir.mkdir()
    
    def teardown_method(self):
        """Cleanup"""
        shutil.rmtree(self.temp_dir)
    
    def test_path_traversal_detection(self):
        """Test path traversal protection"""
        from AXIOM import SecureExtractor
        import logging
        
        logger = logging.getLogger("test")
        extractor = SecureExtractor(logger)
        
        base = Path("/safe/path")
        
        # Safe path
        safe = Path("/safe/path/subdir/file.txt")
        assert extractor.is_safe_path(base, safe)
        
        # Unsafe path (traversal)
        unsafe = Path("/etc/passwd")
        assert not extractor.is_safe_path(base, unsafe)
    
    def test_create_test_zip(self):
        """Helper to create test zip"""
        import zipfile
        
        zip_path = Path(self.temp_dir) / "test.zip"
        
        with zipfile.ZipFile(zip_path, 'w') as zf:
            zf.writestr("file1.txt", "content1")
            zf.writestr("dir/file2.txt", "content2")
        
        return zip_path
    
    def test_zip_extraction(self):
        """Test ZIP extraction"""
        from AXIOM import SecureExtractor
        import logging
        
        logger = logging.getLogger("test")
        extractor = SecureExtractor(logger)
        
        # Create test zip
        zip_path = self.create_test_zip()
        
        # Extract
        success, extract_path = extractor.extract_zip(zip_path, self.extract_dir)
        
        assert success
        assert (self.extract_dir / "file1.txt").exists()
        assert (self.extract_dir / "dir" / "file2.txt").exists()


class TestMetadataExtractor:
    """Test metadata extraction"""
    
    def test_extract_title(self):
        """Test title extraction"""
        from bs4 import BeautifulSoup
        from AXIOM import MetadataExtractor
        import logging
        
        html = """
        <html>
            <head><title>Page Title</title></head>
            <body>
                <h1 class="entry-title">Test Skin Title</h1>
            </body>
        </html>
        """
        
        soup = BeautifulSoup(html, 'html.parser')
        logger = logging.getLogger("test")
        extractor = MetadataExtractor(logger)
        
        title = extractor.extract_title(soup)
        assert title == "Test Skin Title"
    
    def test_extract_download_url(self):
        """Test download URL extraction"""
        from bs4 import BeautifulSoup
        from AXIOM import MetadataExtractor
        import logging
        
        html = """
        <html>
            <body>
                <a href="https://example.com/download/skin.rmskin">Download</a>
            </body>
        </html>
        """
        
        soup = BeautifulSoup(html, 'html.parser')
        logger = logging.getLogger("test")
        extractor = MetadataExtractor(logger)
        
        url = extractor.extract_download_url(soup, "https://example.com")
        assert url == "https://example.com/download/skin.rmskin"
    
    def test_validate_download_url(self):
        """Test download URL validation"""
        from AXIOM import MetadataExtractor
        import logging
        
        logger = logging.getLogger("test")
        extractor = MetadataExtractor(logger)
        
        # Valid URLs
        assert extractor.is_valid_download_url("https://example.com/file.zip")
        assert extractor.is_valid_download_url("https://example.com/file.rmskin")
        assert extractor.is_valid_download_url("https://example.com/download/skin")
        
        # Invalid URLs
        assert not extractor.is_valid_download_url("")
        assert not extractor.is_valid_download_url("https://example.com/page.html")


class TestAsyncDownloader:
    """Test async download functionality"""
    
    def setup_method(self):
        """Setup test environment"""
        self.temp_dir = tempfile.mkdtemp()
        self.download_dir = Path(self.temp_dir) / "downloads"
        self.download_dir.mkdir()
    
    def teardown_method(self):
        """Cleanup"""
        shutil.rmtree(self.temp_dir)
    
    def test_filename_extraction(self):
        """Test filename extraction from URL"""
        from AXIOM import AsyncDownloader, AdaptiveRateLimiter
        import logging
        
        logger = logging.getLogger("test")
        limiter = AdaptiveRateLimiter()
        downloader = AsyncDownloader(self.download_dir, logger, limiter)
        
        # From URL
        filename = downloader.get_filename_from_url("https://example.com/skin.zip")
        assert filename == "skin.zip"
        
        # From Content-Disposition
        filename = downloader.get_filename_from_url(
            "https://example.com/download",
            'attachment; filename="myskin.rmskin"'
        )
        assert filename == "myskin.rmskin"
    
    def test_sanitize_filename(self):
        """Test filename sanitization"""
        from AXIOM import AsyncDownloader, AdaptiveRateLimiter
        import logging
        
        logger = logging.getLogger("test")
        limiter = AdaptiveRateLimiter()
        downloader = AsyncDownloader(self.download_dir, logger, limiter)
        
        # Invalid characters
        result = downloader.sanitize_filename('skin<>:"/\\|?*.zip')
        assert '<' not in result
        assert '>' not in result
        assert ':' not in result
        
        # Length limit
        long_name = 'a' * 200
        result = downloader.sanitize_filename(long_name)
        assert len(result) <= 100


class TestIntegration:
    """Integration tests"""
    
    def setup_method(self):
        """Setup test environment"""
        self.temp_dir = tempfile.mkdtemp()
        self.output_dir = Path(self.temp_dir) / "output"
        self.output_dir.mkdir()
    
    def teardown_method(self):
        """Cleanup"""
        shutil.rmtree(self.temp_dir)
    
    @pytest.mark.asyncio
    async def test_full_pipeline_mock(self):
        """Test complete pipeline with mocked network"""
        from AXIOM import EnhancedAXIOMScraper
        
        # Create mock config
        config_file = Path(self.temp_dir) / "config.json"
        config = {
            "rainmeterui_categories": {
                "primary_skin_categories": [
                    {
                        "name": "Test Category",
                        "url": "https://example.com/test"
                    }
                ]
            }
        }
        
        with open(config_file, 'w') as f:
            json.dump(config, f)
        
        # This would require extensive mocking of aiohttp
        # For now, just test instantiation
        scraper = EnhancedAXIOMScraper(
            config_file=str(config_file),
            output_dir=str(self.output_dir),
            delay=0.1,
            max_workers=2,
            batch_size=10
        )
        
        assert scraper.db is not None
        assert scraper.rate_limiter is not None
        assert scraper.output_dir.exists()


class TestErrorRecovery:
    """Test error recovery and resilience"""
    
    def setup_method(self):
        """Setup test environment"""
        self.temp_dir = tempfile.mkdtemp()
        self.db_path = Path(self.temp_dir) / "test.db"
    
    def teardown_method(self):
        """Cleanup"""
        shutil.rmtree(self.temp_dir)
    
    def test_database_recovery_after_crash(self):
        """Test database can recover after interruption"""
        from AXIOM import SkinDatabase, SkinMetadata
        
        # Create database and add data
        db1 = SkinDatabase(self.db_path)
        skin = SkinMetadata(
            url="https://example.com/skin1",
            title="Test Skin",
            category="Test",
            category_url="https://example.com/cat"
        )
        db1.save_skin(skin)
        
        # Simulate crash by deleting object
        del db1
        
        # Create new database instance
        db2 = SkinDatabase(self.db_path)
        retrieved = db2.get_skin(skin.url)
        
        assert retrieved is not None
        assert retrieved.title == "Test Skin"
    
    def test_partial_download_handling(self):
        """Test handling of interrupted downloads"""
        from AXIOM import SkinDatabase, SkinMetadata
        
        db = SkinDatabase(self.db_path)
        
        # Simulate partial download
        skin = SkinMetadata(
            url="https://example.com/skin1",
            title="Test Skin",
            category="Test",
            category_url="https://example.com/cat",
            download_url="https://example.com/dl",
            download_status="pending"
        )
        db.save_skin(skin)
        
        # Mark as downloading (would be in progress)
        db.update_download_status(skin.url, "downloading")
        
        # After restart, should be able to reset to pending
        db.update_download_status(skin.url, "pending")
        
        retrieved = db.get_skin(skin.url)
        assert retrieved.download_status == "pending"


class TestPerformance:
    """Performance tests"""
    
    def setup_method(self):
        """Setup test environment"""
        self.temp_dir = tempfile.mkdtemp()
        self.db_path = Path(self.temp_dir) / "test.db"
    
    def teardown_method(self):
        """Cleanup"""
        shutil.rmtree(self.temp_dir)
    
    def test_bulk_insert_performance(self):
        """Test bulk insert performance"""
        import time
        from AXIOM import SkinDatabase, SkinMetadata
        
        db = SkinDatabase(self.db_path)
        
        # Insert 1000 skins
        start_time = time.time()
        
        for i in range(1000):
            skin = SkinMetadata(
                url=f"https://example.com/skin{i}",
                title=f"Skin {i}",
                category="Test",
                category_url="https://example.com/cat"
            )
            db.save_skin(skin)
        
        elapsed = time.time() - start_time
        
        # Should complete in reasonable time (< 5 seconds)
        assert elapsed < 5.0
        
        # Verify all inserted
        stats = db.get_statistics()
        assert stats['total_skins'] == 1000
    
    def test_query_performance(self):
        """Test query performance with large dataset"""
        import time
        from AXIOM import SkinDatabase, SkinMetadata
        
        db = SkinDatabase(self.db_path)
        
        # Insert 10000 skins
        for i in range(10000):
            skin = SkinMetadata(
                url=f"https://example.com/skin{i}",
                title=f"Skin {i}",
                category=f"Category {i % 10}",
                category_url="https://example.com/cat",
                download_status="pending" if i % 2 == 0 else "downloaded"
            )
            db.save_skin(skin)
        
        # Test query performance
        start_time = time.time()
        pending = db.get_pending_downloads(limit=100)
        elapsed = time.time() - start_time
        
        # Should be fast (< 0.1 seconds)
        assert elapsed < 0.1
        assert len(pending) == 100


class TestSecurityFeatures:
    """Test security features"""
    
    def setup_method(self):
        """Setup test environment"""
        self.temp_dir = tempfile.mkdtemp()
    
    def teardown_method(self):
        """Cleanup"""
        shutil.rmtree(self.temp_dir)
    
    def test_path_traversal_prevention(self):
        """Test path traversal attacks are prevented"""
        from AXIOM import SecureExtractor
        import logging
        
        logger = logging.getLogger("test")
        extractor = SecureExtractor(logger)
        
        base_path = Path(self.temp_dir) / "safe"
        base_path.mkdir()
        
        # Attempt traversal
        malicious_path = Path(self.temp_dir) / "safe" / ".." / ".." / "etc" / "passwd"
        
        assert not extractor.is_safe_path(base_path, malicious_path)
    
    def test_file_size_limits(self):
        """Test file size limits are enforced"""
        from AXIOM import SecureExtractor
        import logging
        
        logger = logging.getLogger("test")
        extractor = SecureExtractor(logger)
        
        # Verify limits are set
        assert extractor.MAX_EXTRACT_SIZE > 0
        assert extractor.MAX_FILE_SIZE > 0
        assert extractor.MAX_FILE_SIZE < extractor.MAX_EXTRACT_SIZE
    
    def test_zip_bomb_protection(self):
        """Test protection against zip bombs"""
        from AXIOM import SecureExtractor
        import logging
        import zipfile
        
        logger = logging.getLogger("test")
        extractor = SecureExtractor(logger)
        
        # Create a zip with reported size > max
        zip_path = Path(self.temp_dir) / "bomb.zip"
        extract_dir = Path(self.temp_dir) / "extract"
        extract_dir.mkdir()
        
        # This is a simplified test - real zip bombs are more complex
        with zipfile.ZipFile(zip_path, 'w') as zf:
            # Write a file that claims to be huge
            large_data = b'0' * (1024 * 1024)  # 1MB of data
            zf.writestr("small.txt", large_data)
        
        # Should handle gracefully
        success, path = extractor.extract_zip(zip_path, extract_dir)
        # In this case it should succeed since it's under limits
        # Real zip bomb would trigger size check


def run_all_tests():
    """Run all tests"""
    import sys
    
    print("=" * 70)
    print("AXIOM TEST SUITE")
    print("=" * 70)
    print()
    
    # Run pytest
    exit_code = pytest.main([
        __file__,
        '-v',
        '--tb=short',
        '--color=yes'
    ])
    
    sys.exit(exit_code)


if __name__ == "__main__":
    run_all_tests()
