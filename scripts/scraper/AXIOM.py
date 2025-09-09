#!/usr/bin/env python3
"""
🤖 AXIOM - Automated eXtraction Index Operator 🤖

█████╗ ██╗  ██╗██╗ ██████╗ ███╗   ███╗
██╔══██╗╚██╗██╔╝██║██╔═══██╗████╗ ████║
███████║ ╚███╔╝ ██║██║   ██║██╔████╔██║
██╔══██║ ██╔██╗ ██║██║   ██║██║╚██╔╝██║
██║  ██║██╔╝ ██╗██║╚██████╔╝██║ ╚═╝ ██║
╚═╝  ╚═╝╚═╝  ╚═╝╚═╝ ╚═════╝ ╚═╝     ╚═╝

RainmeterUI Complete Collection System - Step 2 Enhanced

AXIOM Mission Profile:
1. 🔍 Scrapes all skin metadata from individual pages
2. 📦 Downloads skin packages (.rmskin, .zip, .rar files)
3. 🗃️  Extracts downloaded packages automatically
4. 📁 Organizes all assets by category and skin name
5. 💾 Creates a complete offline collection of Rainmeter skins

AXIOM Capabilities:
- Complete pipeline from category pages to extracted skins
- Handles multiple download formats (.rmskin, .zip, .rar, .7z)
- Automatic extraction and organization
- Metadata collection (description, author, screenshots, etc.)
- Robust error handling and resume capability
- Progress tracking and detailed logging
- Parallel processing for maximum efficiency

🎯 AXIOM: Your automated Rainmeter skin collection specialist!
"""

import json
import time
import requests
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
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
import zipfile
import rarfile
import py7zr
import shutil
import hashlib
import mimetypes
from datetime import datetime
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
import queue
import math
import random

# Animation and sound imports
try:
    import pygame
    PYGAME_AVAILABLE = True
except ImportError:
    PYGAME_AVAILABLE = False

try:
    import numpy as np
    NUMPY_AVAILABLE = True
except ImportError:
    NUMPY_AVAILABLE = False


class AXIOMBrandingSystem:
    """🎭 AXIOM Visual and Audio Branding System 🎭"""
    
    def __init__(self, enable_animation=True, enable_sound=True):
        self.enable_animation = enable_animation and PYGAME_AVAILABLE
        self.enable_sound = enable_sound and PYGAME_AVAILABLE
        self.screen = None
        self.clock = None
        
        if self.enable_animation or self.enable_sound:
            self.init_pygame()
    
    def init_pygame(self):
        """Initialize pygame for graphics and sound"""
        try:
            pygame.init()
            if self.enable_sound:
                pygame.mixer.init(frequency=22050, size=-16, channels=2, buffer=512)
            
            if self.enable_animation:
                # Create a high-resolution display
                self.screen_width = 1200
                self.screen_height = 800
                self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
                pygame.display.set_caption("🤖 AXIOM - Automated eXtraction Index Operator")
                self.clock = pygame.time.Clock()
                
        except Exception as e:
            print(f"⚠️  Could not initialize pygame: {e}")
            self.enable_animation = False
            self.enable_sound = False
    
    def generate_startup_sound(self):
        """🔊 Generate AXIOM's iconic startup sound"""
        if not self.enable_sound:
            return
        
        try:
            # Create AXIOM's signature sound - a sci-fi activation sequence
            sample_rate = 22050
            duration = 2.5  # seconds
            
            # Generate sound data
            frames = int(duration * sample_rate)
            arr = np.zeros((frames, 2))  # Stereo
            
            # Phase 1: Power-up sweep (0.0 - 0.8s)
            for i in range(int(0.8 * sample_rate)):
                t = i / sample_rate
                # Rising frequency sweep with harmonic overtones
                freq1 = 150 + (t / 0.8) * 200  # Base frequency sweep
                freq2 = freq1 * 1.5  # Fifth harmonic
                freq3 = freq1 * 2.0  # Octave
                
                # Multiple oscillators for rich sound
                wave1 = 0.3 * np.sin(2 * math.pi * freq1 * t)
                wave2 = 0.2 * np.sin(2 * math.pi * freq2 * t)
                wave3 = 0.1 * np.sin(2 * math.pi * freq3 * t)
                
                # Add some digital artifacts for sci-fi feel
                digital_noise = 0.05 * np.sin(2 * math.pi * 800 * t) * (t % 0.1 < 0.05)
                
                # Envelope: fade in and add punch
                envelope = min(1.0, t * 3) * (1 - 0.3 * (t / 0.8))
                
                sample = (wave1 + wave2 + wave3 + digital_noise) * envelope
                arr[i] = [sample, sample * 0.8]  # Slight stereo separation
            
            # Phase 2: Activation confirmation (0.8 - 1.3s)
            for i in range(int(0.8 * sample_rate), int(1.3 * sample_rate)):
                t = (i - int(0.8 * sample_rate)) / sample_rate
                # Triple beep confirmation
                beep_freq = 440  # A4 note
                beep_duration = 0.1
                beep_gap = 0.05
                
                beep_pos = t % (beep_duration + beep_gap)
                if beep_pos < beep_duration:
                    wave = 0.4 * np.sin(2 * math.pi * beep_freq * beep_pos)
                    # Add harmonic for richness
                    wave += 0.2 * np.sin(2 * math.pi * beep_freq * 2 * beep_pos)
                    envelope = np.sin(math.pi * beep_pos / beep_duration)
                    sample = wave * envelope
                else:
                    sample = 0
                
                arr[i] = [sample, sample]
            
            # Phase 3: Ready tone (1.3 - 2.5s)
            for i in range(int(1.3 * sample_rate), frames):
                t = (i - int(1.3 * sample_rate)) / sample_rate
                # Sustained chord with slow fade
                chord_freqs = [261.63, 329.63, 392.00]  # C major chord
                sample = 0
                
                for freq in chord_freqs:
                    sample += 0.15 * np.sin(2 * math.pi * freq * t)
                
                # Fade out envelope
                envelope = max(0, 1 - t / 1.2)
                sample *= envelope
                
                arr[i] = [sample, sample]
            
            # Convert to pygame sound format
            arr = (arr * 32767).astype(np.int16)
            sound = pygame.sndarray.make_sound(arr)
            
            return sound
            
        except Exception as e:
            print(f"⚠️  Could not generate startup sound: {e}")
            return None
    
    def create_water_drop_animation(self):
        """💧 Create stunning 4K water drop animation inspired by high-speed photography"""
        if not self.enable_animation:
            return
        
        # 4K-inspired color palette - based on Markus Reugels' work
        DEEP_BLACK = (8, 12, 20)           # Ultra-deep background
        WATER_BASE = (20, 35, 60)          # Deep water base
        WATER_MID = (40, 80, 140)          # Mid-tone water
        WATER_BRIGHT = (80, 160, 255)      # Bright water highlights
        CRYSTAL_CLEAR = (180, 220, 255)    # Crystal clear water
        PURE_WHITE = (255, 255, 255)       # Pure highlights
        AXIOM_CYAN = (0, 255, 255)         # Signature AXIOM color
        ELECTRIC_BLUE = (100, 200, 255)    # Electric highlights
        REFRACTION = (220, 240, 255)       # Light refraction
        
        # High-resolution animation parameters
        center_x = self.screen_width // 2
        center_y = self.screen_height // 2
        max_radius = 400
        
        # Particle system for micro-droplets
        particles = []
        secondary_drops = []
        
        # Play startup sound
        startup_sound = self.generate_startup_sound()
        if startup_sound:
            startup_sound.play()
        
        animation_time = 0
        total_duration = 3.0  # 3 seconds total
        
        while animation_time < total_duration:
            dt = self.clock.tick(60) / 1000.0  # 60 FPS
            animation_time += dt
            
            # Handle events
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    return
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_SPACE or event.key == pygame.K_ESCAPE:
                        return  # Skip animation
            
            # Clear screen with deep blue background
            self.screen.fill(DEEP_BLUE)
            
            # Phase 1: Water drop formation and fall (0-1.5s)
            if animation_time < 1.5:
                progress = animation_time / 1.5
                
                # Water drop formation at top
                drop_y = int(50 + progress * (center_y - 100))
                drop_size = int(15 + 10 * math.sin(progress * math.pi))
                
                # Draw the falling water drop with gradient effect
                for r in range(drop_size, 0, -1):
                    alpha = int(255 * (r / drop_size))
                    color_intensity = r / drop_size
                    color = (
                        int(LIGHT_BLUE[0] * color_intensity),
                        int(LIGHT_BLUE[1] * color_intensity),
                        int(LIGHT_BLUE[2] * color_intensity)
                    )
                    pygame.draw.circle(self.screen, color, (center_x, drop_y), r)
                
                # Add shine effect to drop
                shine_x = center_x - drop_size // 3
                shine_y = drop_y - drop_size // 3
                # Soft sterile grey reflection
                SOFT_GREY = (200, 200, 200)
                pygame.draw.circle(self.screen, SOFT_GREY, (shine_x, shine_y), drop_size // 4)
            
            # Phase 2: Impact and ripple formation (1.5-3.0s)
            else:
                impact_time = animation_time - 1.5
                impact_progress = impact_time / 1.5
                
                # Multiple ripple waves
                num_ripples = 5
                for ripple_idx in range(num_ripples):
                    ripple_delay = ripple_idx * 0.2
                    if impact_time > ripple_delay:
                        ripple_time = impact_time - ripple_delay
                        ripple_radius = int(ripple_time * 150)
                        
                        if ripple_radius < max_radius:
                            # Ripple intensity decreases over time and distance
                            intensity = max(0, 1 - ripple_time / 2.0)
                            intensity *= max(0, 1 - ripple_radius / max_radius)
                            
                            # Draw ripple ring with multiple circles for thickness
                            for thickness in range(5):
                                alpha = int(255 * intensity * (1 - thickness / 5))
                                if alpha > 10:
                                    color = (
                                        int(AXIOM_CYAN[0] * intensity),
                                        int(AXIOM_CYAN[1] * intensity),
                                        int(AXIOM_CYAN[2] * intensity)
                                    )
                                    pygame.draw.circle(self.screen, color, 
                                                     (center_x, center_y), 
                                                     ripple_radius + thickness, 2)
                
                # Impact splash effect
                if impact_time < 0.3:
                    splash_progress = impact_time / 0.3
                    num_particles = 20
                    
                    for i in range(num_particles):
                        angle = (2 * math.pi * i) / num_particles
                        distance = splash_progress * 80
                        
                        particle_x = center_x + int(math.cos(angle) * distance)
                        particle_y = center_y + int(math.sin(angle) * distance * 0.5)  # Flatten vertically
                        
                        particle_size = int(8 * (1 - splash_progress))
                        if particle_size > 1:
                            pygame.draw.circle(self.screen, LIGHT_BLUE, 
                                             (particle_x, particle_y), particle_size)
            
            # AXIOM logo overlay (appears after 1 second)
            if animation_time > 1.0:
                logo_alpha = min(255, int(255 * (animation_time - 1.0) / 0.5))
                
                # Draw AXIOM text with glow effect
                font_size = 72
                try:
                    font = pygame.font.Font(None, font_size)
                    text = "AXIOM"
                    
                    # Glow effect - multiple renders with decreasing intensity
                    for glow in range(5, 0, -1):
                        glow_color = (0, int(logo_alpha * glow / 5), int(logo_alpha * glow / 5))
                        glow_surface = font.render(text, True, glow_color)
                        text_rect = glow_surface.get_rect(center=(center_x, center_y + 150))
                        
                        for dx in range(-glow, glow + 1):
                            for dy in range(-glow, glow + 1):
                                self.screen.blit(glow_surface, (text_rect.x + dx, text_rect.y + dy))
                    
                    # Main text
                    main_color = (255, 255, 255, logo_alpha)
                    text_surface = font.render(text, True, (255, 255, 255))
                    text_rect = text_surface.get_rect(center=(center_x, center_y + 150))
                    self.screen.blit(text_surface, text_rect)
                    
                    # Subtitle
                    subtitle_font = pygame.font.Font(None, 32)
                    subtitle = "Automated eXtraction Index Operator"
                    subtitle_surface = subtitle_font.render(subtitle, True, AXIOM_CYAN)
                    subtitle_rect = subtitle_surface.get_rect(center=(center_x, center_y + 200))
                    self.screen.blit(subtitle_surface, subtitle_rect)
                    
                except:
                    # Fallback if font loading fails
                    pass
            
            # Progress indicator
            progress_width = int((self.screen_width * 0.6) * (animation_time / total_duration))
            progress_rect = pygame.Rect((self.screen_width - int(self.screen_width * 0.6)) // 2, 
                                      self.screen_height - 50, progress_width, 4)
            pygame.draw.rect(self.screen, AXIOM_CYAN, progress_rect)
            
            # Instructions (fade in after 2 seconds)
            if animation_time > 2.0:
                instruction_alpha = min(255, int(255 * (animation_time - 2.0) / 1.0))
                try:
                    instruction_font = pygame.font.Font(None, 24)
                    instruction_text = "Press SPACE or ESC to continue..."
                    instruction_color = (200, 200, 200)
                    instruction_surface = instruction_font.render(instruction_text, True, instruction_color)
                    instruction_rect = instruction_surface.get_rect(center=(center_x, self.screen_height - 100))
                    self.screen.blit(instruction_surface, instruction_rect)
                except:
                    pass
            
            pygame.display.flip()
        
        # Fade out
        for fade_step in range(30):
            fade_surface = pygame.Surface((self.screen_width, self.screen_height))
            fade_surface.set_alpha(fade_step * 8)
            fade_surface.fill((0, 0, 0))
            self.screen.blit(fade_surface, (0, 0))
            pygame.display.flip()
            self.clock.tick(60)
    
    def play_operation_sound(self, sound_type="success"):
        """🔊 Play operation feedback sounds"""
        if not self.enable_sound:
            return
        
        try:
            sample_rate = 22050
            duration = 0.5
            frames = int(duration * sample_rate)
            arr = np.zeros(frames)
            
            if sound_type == "success":
                # Success: Rising major chord
                freqs = [523.25, 659.25, 783.99]  # C5, E5, G5
                for i, freq in enumerate(freqs):
                    for j in range(frames):
                        t = j / sample_rate
                        envelope = max(0, 1 - t / duration)
                        arr[j] += 0.2 * np.sin(2 * math.pi * freq * t) * envelope
            
            elif sound_type == "error":
                # Error: Descending minor chord
                freqs = [440, 415.3, 369.99]  # A4, Ab4, Gb4
                for i, freq in enumerate(freqs):
                    for j in range(frames):
                        t = j / sample_rate
                        envelope = max(0, 1 - t / duration)
                        arr[j] += 0.2 * np.sin(2 * math.pi * freq * t) * envelope
            
            elif sound_type == "processing":
                # Processing: Gentle pulse
                base_freq = 220
                for j in range(frames):
                    t = j / sample_rate
                    pulse = 0.5 + 0.5 * np.sin(2 * math.pi * 2 * t)  # 2 Hz pulse
                    arr[j] = 0.1 * np.sin(2 * math.pi * base_freq * t) * pulse
            
            # Convert to pygame sound
            arr = (arr * 32767).astype(np.int16)
            sound = pygame.sndarray.make_sound(arr)
            sound.play()
            
        except Exception as e:
            print(f"⚠️  Could not play operation sound: {e}")
    
    def show_startup_sequence(self):
        """🚀 Complete AXIOM startup experience"""
        print("\n" + "=" * 70)
        print("🤖 AXIOM STARTUP SEQUENCE INITIATED 🤖")
        print("=" * 70)
        
        if self.enable_animation:
            print("🎬 Launching visual interface...")
            self.create_water_drop_animation()
            
            # Cleanup pygame after animation
            if self.screen:
                pygame.quit()
        else:
            # ASCII animation fallback
            self.ascii_startup_animation()
        
        print("\n✅ AXIOM fully operational and ready for data extraction!")
        print("=" * 70 + "\n")
    
    def ascii_startup_animation(self):
        """🎨 Fallback ASCII animation when pygame is not available"""
        frames = [
            """
    ╔═══════════════════════════════════════╗
    ║              AXIOM v2.0               ║
    ║     Automated eXtraction Index        ║
    ║           Operator                    ║
    ╚═══════════════════════════════════════╝
                  [        ]
            """,
            """
    ╔═══════════════════════════════════════╗
    ║              AXIOM v2.0               ║
    ║     Automated eXtraction Index        ║
    ║           Operator                    ║
    ╚═══════════════════════════════════════╝
                  [▓       ]
            """,
            """
    ╔═══════════════════════════════════════╗
    ║              AXIOM v2.0               ║
    ║     Automated eXtraction Index        ║
    ║           Operator                    ║
    ╚═══════════════════════════════════════╝
                  [▓▓▓     ]
            """,
            """
    ╔═══════════════════════════════════════╗
    ║              AXIOM v2.0               ║
    ║     Automated eXtraction Index        ║
    ║           Operator                    ║
    ╚═══════════════════════════════════════╝
                  [▓▓▓▓▓▓  ]
            """,
            """
    ╔═══════════════════════════════════════╗
    ║              AXIOM v2.0               ║
    ║     Automated eXtraction Index        ║
    ║           Operator                    ║
    ╚═══════════════════════════════════════╝
                  [▓▓▓▓▓▓▓▓]
            💧 READY FOR EXTRACTION 💧
            """
        ]
        
        for frame in frames:
            if os.name == 'nt':  # Windows
                os.system('cls')
            else:  # Unix/Linux/MacOS
                os.system('clear')
            
            print(frame)
            time.sleep(0.8)
        
        time.sleep(1.0)
    
    def cleanup(self):
        """🧹 Clean up pygame resources"""
        if PYGAME_AVAILABLE and pygame.get_init():
            pygame.quit()


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
    tags: List[str] = None
    screenshots: List[str] = None
    created_date: str = ""
    updated_date: str = ""
    version: str = ""
    compatibility: str = ""
    scraped_at: str = ""
    download_status: str = "pending"  # pending, downloaded, extracted, failed
    local_path: str = ""
    extracted_path: str = ""
    file_hash: str = ""
    
    def __post_init__(self):
        if self.tags is None:
            self.tags = []
        if self.screenshots is None:
            self.screenshots = []
        if not self.scraped_at:
            self.scraped_at = datetime.now().isoformat()
    
    def __hash__(self):
        return hash(self.url)
    
    def __eq__(self, other):
        if isinstance(other, SkinMetadata):
            return self.url == other.url
        return False


class DownloadExtractor:
    """Handles downloading and extracting skin packages"""
    
    def __init__(self, download_dir: Path, extract_dir: Path, logger):
        self.download_dir = download_dir
        self.extract_dir = extract_dir
        self.logger = logger
        
        # Create directories
        self.download_dir.mkdir(parents=True, exist_ok=True)
        self.extract_dir.mkdir(parents=True, exist_ok=True)
        
        # Setup session for downloads
        self.session = requests.Session()
        retry_strategy = Retry(
            total=3,
            backoff_factor=2,
            status_forcelist=[429, 500, 502, 503, 504],
        )
        adapter = HTTPAdapter(max_retries=retry_strategy)
        self.session.mount("http://", adapter)
        self.session.mount("https://", adapter)
        
        self.session.headers.update({
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
        })
    
    def get_filename_from_url(self, url: str, content_disposition: str = None) -> str:
        """Extract filename from URL or Content-Disposition header"""
        # Try Content-Disposition first
        if content_disposition:
            import re
            cd_match = re.search(r'filename[*]?=(?:UTF-8\'\')?["\']?([^"\';]+)["\']?', content_disposition)
            if cd_match:
                filename = unquote(cd_match.group(1))
                if filename and not filename.startswith('.'):
                    return filename
        
        # Extract from URL
        parsed = urlparse(url)
        filename = Path(parsed.path).name
        
        if filename and '.' in filename:
            return unquote(filename)
        
        # Generate default filename
        url_hash = hashlib.md5(url.encode()).hexdigest()[:8]
        return f"skin_{url_hash}.zip"
    
    def download_file(self, url: str, category: str, skin_name: str) -> Tuple[bool, str, str]:
        """
        Download a file from URL
        
        Returns:
            (success, local_path, file_hash)
        """
        try:
            self.logger.info(f"Downloading: {url}")
            
            # Create category directory
            category_dir = self.download_dir / self.sanitize_filename(category)
            category_dir.mkdir(exist_ok=True)
            
            # Start download
            response = self.session.get(url, stream=True, timeout=60)
            response.raise_for_status()
            
            # Get filename
            content_disposition = response.headers.get('content-disposition')
            filename = self.get_filename_from_url(url, content_disposition)
            
            # Ensure proper extension
            if not any(filename.lower().endswith(ext) for ext in ['.zip', '.rar', '.7z', '.rmskin']):
                # Try to detect from content-type
                content_type = response.headers.get('content-type', '').lower()
                if 'zip' in content_type:
                    filename += '.zip'
                elif 'rar' in content_type:
                    filename += '.rar'
                else:
                    filename += '.zip'  # Default
            
            # Create full path
            safe_skin_name = self.sanitize_filename(skin_name)
            local_path = category_dir / f"{safe_skin_name}_{filename}"
            
            # Download with progress
            total_size = int(response.headers.get('content-length', 0))
            downloaded = 0
            
            hasher = hashlib.sha256()
            
            with open(local_path, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    if chunk:
                        f.write(chunk)
                        hasher.update(chunk)
                        downloaded += len(chunk)
                        
                        if total_size > 0:
                            progress = (downloaded / total_size) * 100
                            if downloaded % (1024 * 1024) == 0:  # Log every MB
                                self.logger.debug(f"Downloaded {downloaded}/{total_size} bytes ({progress:.1f}%)")
            
            file_hash = hasher.hexdigest()
            self.logger.info(f"Downloaded successfully: {local_path} ({downloaded} bytes)")
            
            return True, str(local_path), file_hash
            
        except Exception as e:
            self.logger.error(f"Download failed for {url}: {e}")
            return False, "", ""
    
    def extract_archive(self, archive_path: str, category: str, skin_name: str) -> Tuple[bool, str]:
        """
        Extract archive to organized directory structure
        
        Returns:
            (success, extract_path)
        """
        try:
            archive_path = Path(archive_path)
            
            if not archive_path.exists():
                self.logger.error(f"Archive not found: {archive_path}")
                return False, ""
            
            # Create extraction directory
            category_dir = self.extract_dir / self.sanitize_filename(category)
            skin_dir = category_dir / self.sanitize_filename(skin_name)
            skin_dir.mkdir(parents=True, exist_ok=True)
            
            self.logger.info(f"Extracting {archive_path} to {skin_dir}")
            
            # Determine archive type and extract
            file_ext = archive_path.suffix.lower()
            
            if file_ext in ['.zip', '.rmskin']:
                return self.extract_zip(archive_path, skin_dir)
            elif file_ext == '.rar':
                return self.extract_rar(archive_path, skin_dir)
            elif file_ext == '.7z':
                return self.extract_7z(archive_path, skin_dir)
            else:
                self.logger.error(f"Unsupported archive format: {file_ext}")
                return False, ""
                
        except Exception as e:
            self.logger.error(f"Extraction failed for {archive_path}: {e}")
            return False, ""
    
    def extract_zip(self, archive_path: Path, extract_dir: Path) -> Tuple[bool, str]:
        """Extract ZIP file"""
        try:
            with zipfile.ZipFile(archive_path, 'r') as zip_ref:
                # Get list of files
                file_list = zip_ref.namelist()
                self.logger.debug(f"ZIP contains {len(file_list)} files")
                
                # Extract all files
                zip_ref.extractall(extract_dir)
                
                self.logger.info(f"Successfully extracted ZIP: {len(file_list)} files")
                return True, str(extract_dir)
                
        except zipfile.BadZipFile:
            self.logger.error(f"Corrupted ZIP file: {archive_path}")
            return False, ""
        except Exception as e:
            self.logger.error(f"ZIP extraction error: {e}")
            return False, ""
    
    def extract_rar(self, archive_path: Path, extract_dir: Path) -> Tuple[bool, str]:
        """Extract RAR file"""
        try:
            with rarfile.RarFile(archive_path, 'r') as rar_ref:
                file_list = rar_ref.namelist()
                self.logger.debug(f"RAR contains {len(file_list)} files")
                
                rar_ref.extractall(extract_dir)
                
                self.logger.info(f"Successfully extracted RAR: {len(file_list)} files")
                return True, str(extract_dir)
                
        except rarfile.BadRarFile:
            self.logger.error(f"Corrupted RAR file: {archive_path}")
            return False, ""
        except Exception as e:
            self.logger.error(f"RAR extraction error: {e}")
            return False, ""
    
    def extract_7z(self, archive_path: Path, extract_dir: Path) -> Tuple[bool, str]:
        """Extract 7Z file"""
        try:
            with py7zr.SevenZipFile(archive_path, 'r') as archive:
                archive.extractall(extract_dir)
                
                self.logger.info(f"Successfully extracted 7Z file")
                return True, str(extract_dir)
                
        except Exception as e:
            self.logger.error(f"7Z extraction error: {e}")
            return False, ""
    
    def sanitize_filename(self, filename: str) -> str:
        """Sanitize filename for filesystem"""
        # Remove/replace invalid characters
        invalid_chars = '<>:"/\\|?*'
        for char in invalid_chars:
            filename = filename.replace(char, '_')
        
        # Limit length
        filename = filename[:100]
        
        # Remove leading/trailing spaces and dots
        filename = filename.strip('. ')
        
        return filename if filename else "unnamed"


class RainmeterCompleteScraper:
    """Complete scraper for RainmeterUI with download and extraction"""
    
    def __init__(self, config_file: str = "rainmeterui_categories.json", 
                 output_dir: str = "scraped_data", 
                 delay: float = 1.0,
                 max_workers: int = 3):
        """
        Initialize the complete scraper
        
        Args:
            config_file: Path to the categories configuration file
            output_dir: Directory to save scraped data
            delay: Delay between requests in seconds
            max_workers: Number of concurrent download threads
        """
        self.config_file = config_file
        self.output_dir = Path(output_dir)
        self.delay = delay
        self.max_workers = max_workers
        
        # Create directory structure
        self.output_dir.mkdir(exist_ok=True)
        self.downloads_dir = self.output_dir / "downloads"
        self.extracted_dir = self.output_dir / "extracted_skins"
        self.metadata_dir = self.output_dir / "metadata"
        
        for dir_path in [self.downloads_dir, self.extracted_dir, self.metadata_dir]:
            dir_path.mkdir(exist_ok=True)
        
        # Setup logging
        self.setup_logging()
        
        # Load configuration
        self.config = self.load_config()
        
        # Setup requests session
        self.session = self.setup_session()
        
        # Initialize download extractor
        self.extractor = DownloadExtractor(
            self.downloads_dir, 
            self.extracted_dir, 
            self.logger
        )
        
        # Storage for scraped data
        self.all_skins: Set[SkinMetadata] = set()
        self.failed_downloads: List[Dict] = []
        
        # Progress tracking
        self.progress_file = self.output_dir / "scraping_progress.json"
        self.load_progress()
        
        # Statistics
        self.stats = {
            'categories_processed': 0,
            'pages_scraped': 0,
            'skins_found': 0,
            'skins_downloaded': 0,
            'skins_extracted': 0,
            'download_failures': 0,
            'extraction_failures': 0,
            'total_size_downloaded': 0,
            'errors': 0
        }
    
    def setup_logging(self):
        """Setup comprehensive logging"""
        log_file = self.output_dir / "complete_scraper.log"
        
        # Create custom formatter
        formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        
        # File handler
        file_handler = logging.FileHandler(log_file, encoding='utf-8')
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(formatter)
        
        # Console handler
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(formatter)
        
        # Setup logger
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(logging.DEBUG)
        self.logger.addHandler(file_handler)
        self.logger.addHandler(console_handler)
    
    def setup_session(self) -> requests.Session:
        """Setup requests session with proper configuration"""
        session = requests.Session()
        
        retry_strategy = Retry(
            total=3,
            backoff_factor=1,
            status_forcelist=[429, 500, 502, 503, 504],
        )
        adapter = HTTPAdapter(max_retries=retry_strategy)
        session.mount("http://", adapter)
        session.mount("https://", adapter)
        
        session.headers.update({
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36',
            'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
            'Accept-Language': 'en-US,en;q=0.9',
            'Accept-Encoding': 'gzip, deflate',
            'Connection': 'keep-alive',
        })
        
        return session
    
    def load_config(self) -> Dict:
        """Load categories configuration"""
        try:
            with open(self.config_file, 'r', encoding='utf-8') as f:
                config = json.load(f)
            self.logger.info(f"Loaded configuration from {self.config_file}")
            return config['rainmeterui_categories']
        except Exception as e:
            self.logger.error(f"Failed to load config file {self.config_file}: {e}")
            raise
    
    def load_progress(self):
        """Load previous scraping progress"""
        if self.progress_file.exists():
            try:
                with open(self.progress_file, 'r', encoding='utf-8') as f:
                    progress_data = json.load(f)
                
                # Load previously scraped skins
                for skin_data in progress_data.get('scraped_skins', []):
                    skin = SkinMetadata(**skin_data)
                    self.all_skins.add(skin)
                
                self.logger.info(f"Loaded {len(self.all_skins)} previously scraped skins")
                
            except Exception as e:
                self.logger.warning(f"Could not load progress file: {e}")
    
    def save_progress(self):
        """Save current scraping progress"""
        try:
            progress_data = {
                'last_updated': datetime.now().isoformat(),
                'statistics': self.stats,
                'scraped_skins': [asdict(skin) for skin in self.all_skins]
            }
            
            with open(self.progress_file, 'w', encoding='utf-8') as f:
                json.dump(progress_data, f, indent=2, ensure_ascii=False)
                
        except Exception as e:
            self.logger.error(f"Failed to save progress: {e}")
    
    def get_page_content(self, url: str) -> Optional[BeautifulSoup]:
        """Fetch and parse page content"""
        try:
            self.logger.info(f"Fetching: {url}")
            response = self.session.get(url, timeout=30)
            response.raise_for_status()
            
            soup = BeautifulSoup(response.content, 'html.parser')
            time.sleep(self.delay)
            
            return soup
            
        except requests.exceptions.RequestException as e:
            self.logger.error(f"Request failed for {url}: {e}")
            self.stats['errors'] += 1
            return None
    
    def extract_skin_metadata(self, soup: BeautifulSoup, url: str, category_name: str, 
                            category_url: str, page_num: int = 1) -> Optional[SkinMetadata]:
        """Extract complete metadata from a skin page"""
        try:
            # Basic info
            title = self.extract_title(soup)
            author = self.extract_author(soup)
            description = self.extract_description(soup)
            
            # Download information
            download_url = self.extract_download_url(soup, url)
            file_size = self.extract_file_size(soup)
            downloads_count = self.extract_downloads_count(soup)
            
            # Additional metadata
            rating = self.extract_rating(soup)
            tags = self.extract_tags(soup)
            screenshots = self.extract_screenshots(soup, url)
            created_date = self.extract_date(soup, 'created')
            updated_date = self.extract_date(soup, 'updated')
            version = self.extract_version(soup)
            compatibility = self.extract_compatibility(soup)
            
            # Create metadata object
            metadata = SkinMetadata(
                url=url,
                title=title,
                category=category_name,
                category_url=category_url,
                page_number=page_num,
                author=author,
                description=description,
                download_url=download_url,
                file_size=file_size,
                downloads_count=downloads_count,
                rating=rating,
                tags=tags,
                screenshots=screenshots,
                created_date=created_date,
                updated_date=updated_date,
                version=version,
                compatibility=compatibility
            )
            
            if download_url:
                metadata.download_filename = self.extractor.get_filename_from_url(download_url)
            
            self.logger.debug(f"Extracted metadata for: {title}")
            return metadata
            
        except Exception as e:
            self.logger.error(f"Failed to extract metadata from {url}: {e}")
            return None
    
    def extract_title(self, soup: BeautifulSoup) -> str:
        """Extract skin title"""
        selectors = [
            'h1.entry-title',
            'h1.post-title', 
            '.entry-header h1',
            'h1',
            'title'
        ]
        
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                title = element.get_text(strip=True)
                if title and len(title) > 3:
                    return title
        
        return "Unknown Skin"
    
    def extract_author(self, soup: BeautifulSoup) -> str:
        """Extract skin author"""
        selectors = [
            '.author a',
            '.post-author a',
            '.entry-meta a[rel="author"]',
            '[class*="author"] a'
        ]
        
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                author = element.get_text(strip=True)
                if author:
                    return author
        
        return "Unknown Author"
    
    def extract_description(self, soup: BeautifulSoup) -> str:
        """Extract skin description"""
        selectors = [
            '.entry-content',
            '.post-content',
            '.content',
            'article .text',
            '.description'
        ]
        
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                # Remove script and style elements
                for script in element(["script", "style"]):
                    script.decompose()
                
                description = element.get_text(strip=True)
                if description and len(description) > 20:
                    return description[:1000]  # Limit length
        
        return ""
    
    def extract_download_url(self, soup: BeautifulSoup, base_url: str) -> str:
        """Extract download URL"""
        selectors = [
            'a[href*="download"]',
            'a[href*=".rmskin"]',
            'a[href*=".zip"]',
            'a[href*=".rar"]',
            '.download-btn',
            '.download-link',
            'a:contains("Download")',
            'a[class*="download"]'
        ]
        
        for selector in selectors:
            elements = soup.select(selector)
            for element in elements:
                href = element.get('href')
                if href:
                    download_url = urljoin(base_url, href)
                    
                    # Validate download URL
                    if self.is_valid_download_url(download_url):
                        return download_url
        
        return ""
    
    def is_valid_download_url(self, url: str) -> bool:
        """Check if URL is a valid download link"""
        if not url:
            return False
        
        parsed = urlparse(url)
        path = parsed.path.lower()
        
        # Check for download file extensions
        download_extensions = ['.rmskin', '.zip', '.rar', '.7z']
        if any(path.endswith(ext) for ext in download_extensions):
            return True
        
        # Check for download indicators in URL
        download_indicators = ['download', 'get', 'file']
        if any(indicator in path for indicator in download_indicators):
            return True
        
        return False
    
    def extract_file_size(self, soup: BeautifulSoup) -> str:
        """Extract file size information"""
        # Look for size information in text
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
        """Extract rating information"""
        selectors = [
            '.rating',
            '.stars',
            '[class*="rating"]'
        ]
        
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                rating_text = element.get_text(strip=True)
                if rating_text:
                    return rating_text
        
        return ""
    
    def extract_tags(self, soup: BeautifulSoup) -> List[str]:
        """Extract tags/categories"""
        tags = []
        
        selectors = [
            '.tags a',
            '.tag-links a',
            '.post-tags a',
            '[class*="tag"] a'
        ]
        
        for selector in selectors:
            elements = soup.select(selector)
            for element in elements:
                tag = element.get_text(strip=True)
                if tag and tag not in tags:
                    tags.append(tag)
        
        return tags
    
    def extract_screenshots(self, soup: BeautifulSoup, base_url: str) -> List[str]:
        """Extract screenshot URLs"""
        screenshots = []
        
        # Look for images in content
        img_elements = soup.select('.entry-content img, .post-content img, article img')
        
        for img in img_elements:
            src = img.get('src') or img.get('data-src')
            if src:
                full_url = urljoin(base_url, src)
                if self.is_screenshot_url(full_url):
                    screenshots.append(full_url)
        
        return screenshots
    
    def is_screenshot_url(self, url: str) -> bool:
        """Check if URL is likely a screenshot"""
        if not url:
            return False
        
        parsed = urlparse(url)
        path = parsed.path.lower()
        
        # Image extensions
        img_extensions = ['.jpg', '.jpeg', '.png', '.gif', '.webp']
        if any(path.endswith(ext) for ext in img_extensions):
            return True
        
        return False
    
    def extract_date(self, soup: BeautifulSoup, date_type: str) -> str:
        """Extract creation or update date"""
        selectors = [
            '.entry-date',
            '.post-date',
            'time',
            '.date'
        ]
        
        for selector in selectors:
            element = soup.select_one(selector)
            if element:
                # Try datetime attribute first
                datetime_attr = element.get('datetime')
                if datetime_attr:
                    return datetime_attr
                
                # Get text content
                date_text = element.get_text(strip=True)
                if date_text:
                    return date_text
        
        return ""
    
    def extract_version(self, soup: BeautifulSoup) -> str:
        """Extract version information"""
        version_pattern = re.compile(r'version\s*:?\s*([0-9.]+)', re.IGNORECASE)
        
        text = soup.get_text()
        match = version_pattern.search(text)
        if match:
            return match.group(1)
        
        return ""
    
    def extract_compatibility(self, soup: BeautifulSoup) -> str:
        """Extract Rainmeter compatibility information"""
        compat_pattern = re.compile(r'rainmeter\s*([0-9.]+)', re.IGNORECASE)
        
        text = soup.get_text()
        match = compat_pattern.search(text)
        if match:
            return f"Rainmeter {match.group(1)}"
        
        return ""
    
    def process_skin_download(self, skin: SkinMetadata) -> SkinMetadata:
        """Download and extract a skin package"""
        if not skin.download_url:
            self.logger.warning(f"No download URL for {skin.title}")
            skin.download_status = "no_download_url"
            return skin
        
        try:
            # Download the file
            success, local_path, file_hash = self.extractor.download_file(
                skin.download_url, 
                skin.category, 
                skin.title
            )
            
            if success:
                skin.local_path = local_path
                skin.file_hash = file_hash
                skin.download_status = "downloaded"
                self.stats['skins_downloaded'] += 1
                
                # Get file size
                file_size = Path(local_path).stat().st_size
                self.stats['total_size_downloaded'] += file_size
                
                # Extract the archive
                extract_success, extract_path = self.extractor.extract_archive(
                    local_path, 
                    skin.category, 
                    skin.title
                )
                
                if extract_success:
                    skin.extracted_path = extract_path
                    skin.download_status = "extracted"
                    self.stats['skins_extracted'] += 1
                    self.logger.info(f"Successfully processed: {skin.title}")
                else:
                    skin.download_status = "extraction_failed"
                    self.stats['extraction_failures'] += 1
                    
            else:
                skin.download_status = "download_failed"
                self.stats['download_failures'] += 1
                self.failed_downloads.append({
                    'skin': skin.title,
                    'url': skin.download_url,
                    'category': skin.category
                })
        
        except Exception as e:
            self.logger.error(f"Error processing download for {skin.title}: {e}")
            skin.download_status = "error"
            self.stats['errors'] += 1
        
        return skin
    
    def scrape_individual_skin_page(self, url: str, category_name: str, 
                                  category_url: str, page_num: int = 1) -> Optional[SkinMetadata]:
        """Scrape an individual skin page for complete metadata"""
        soup = self.get_page_content(url)
        if not soup:
            return None
        
        return self.extract_skin_metadata(soup, url, category_name, category_url, page_num)
    
    def find_skin_page_urls(self, soup: BeautifulSoup, category_url: str) -> List[str]:
        """Find all skin page URLs from a category page"""
        urls = set()
        
        # Common selectors for skin links
        selectors = [
            'article h2 a',
            '.post-title a',
            '.entry-title a',
            'h3 a',
            '.skin-item a',
            'a[href*="rainmeterui.com"]'
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
        """Check if URL is an individual skin page"""
        if not url or not url.startswith('http'):
            return False
        
        parsed = urlparse(url)
        
        if 'rainmeterui.com' not in parsed.netloc.lower():
            return False
        
        path = parsed.path.lower()
        
        # Exclude category/listing pages
        exclude_patterns = [
            '/tag/', '/category/', '/archive/', '/page/', 
            '/author/', '/date/', '/redirect/', '.xml', '.rss'
        ]
        
        if any(pattern in path for pattern in exclude_patterns):
            return False
        
        # Must be a specific post/page
        path_parts = path.strip('/').split('/')
        if len(path_parts) >= 1 and not path.endswith('/'):
            return True
        
        return False
    
    def find_next_page_url(self, soup: BeautifulSoup, current_url: str) -> Optional[str]:
        """Find next page URL for pagination"""
        selectors = [
            'a.next', 'a[rel="next"]', '.next-page a',
            '.pagination-next a', '.nav-next a'
        ]
        
        for selector in selectors:
            next_link = soup.select_one(selector)
            if next_link and next_link.get('href'):
                return urljoin(current_url, next_link.get('href'))
        
        return None
    
    def scrape_category_complete(self, category: Dict) -> List[SkinMetadata]:
        """Completely scrape a category with downloads and extraction"""
        category_name = category.get('name', 'Unknown')
        category_url = category.get('url')
        
        if not category_url:
            self.logger.error(f"No URL for category {category_name}")
            return []
        
        self.logger.info(f"Starting complete scrape of category: {category_name}")
        
        all_skins = []
        current_url = category_url
        page_num = 1
        max_pages = 50
        
        while current_url and page_num <= max_pages:
            self.logger.info(f"Processing {category_name} page {page_num}: {current_url}")
            
            soup = self.get_page_content(current_url)
            if not soup:
                break
            
            # Find all skin page URLs on this category page
            skin_urls = self.find_skin_page_urls(soup, category_url)
            self.logger.info(f"Found {len(skin_urls)} skin URLs on page {page_num}")
            
            # Process each skin page
            for skin_url in skin_urls:
                # Check if already processed
                if any(skin.url == skin_url for skin in self.all_skins):
                    self.logger.debug(f"Skipping already processed: {skin_url}")
                    continue
                
                # Scrape individual skin page
                skin_metadata = self.scrape_individual_skin_page(
                    skin_url, category_name, category_url, page_num
                )
                
                if skin_metadata:
                    all_skins.append(skin_metadata)
                    self.stats['skins_found'] += 1
                    
                    # Add small delay between skin pages
                    time.sleep(self.delay * 0.5)
            
            self.stats['pages_scraped'] += 1
            
            # Find next page
            next_url = self.find_next_page_url(soup, current_url)
            if next_url and next_url != current_url:
                current_url = next_url
                page_num += 1
            else:
                break
        
        self.logger.info(f"Completed scraping {category_name}: {len(all_skins)} skins found")
        return all_skins
    
    def download_all_skins_parallel(self, skins: List[SkinMetadata]) -> List[SkinMetadata]:
        """Download all skins using parallel processing"""
        self.logger.info(f"Starting parallel download of {len(skins)} skins")
        
        # Filter skins that need downloading
        to_download = [skin for skin in skins if skin.download_url and 
                      skin.download_status == "pending"]
        
        if not to_download:
            self.logger.info("No skins need downloading")
            return skins
        
        self.logger.info(f"Downloading {len(to_download)} skins with {self.max_workers} workers")
        
        completed_skins = []
        
        with ThreadPoolExecutor(max_workers=self.max_workers) as executor:
            # Submit all download tasks
            future_to_skin = {
                executor.submit(self.process_skin_download, skin): skin 
                for skin in to_download
            }
            
            # Process completed downloads
            for future in as_completed(future_to_skin):
                skin = future_to_skin[future]
                try:
                    processed_skin = future.result()
                    completed_skins.append(processed_skin)
                    
                    # Save progress periodically
                    if len(completed_skins) % 10 == 0:
                        self.save_progress()
                        self.logger.info(f"Progress: {len(completed_skins)}/{len(to_download)} downloads completed")
                
                except Exception as e:
                    self.logger.error(f"Download failed for {skin.title}: {e}")
                    skin.download_status = "error"
                    completed_skins.append(skin)
        
        # Merge with non-downloaded skins
        all_processed = []
        processed_urls = {skin.url for skin in completed_skins}
        
        for skin in skins:
            if skin.url in processed_urls:
                # Find the processed version
                processed_skin = next(s for s in completed_skins if s.url == skin.url)
                all_processed.append(processed_skin)
            else:
                all_processed.append(skin)
        
        return all_processed
    
    def save_complete_results(self):
        """Save all collected data and results"""
        self.logger.info("Saving complete results...")
        
        # Convert to list for serialization
        skins_list = list(self.all_skins)
        
        # Main results file
        results_file = self.output_dir / "complete_skin_collection.json"
        results_data = {
            'scraping_completed': datetime.now().isoformat(),
            'total_skins': len(skins_list),
            'statistics': self.stats,
            'failed_downloads': self.failed_downloads,
            'skins': [asdict(skin) for skin in skins_list]
        }
        
        with open(results_file, 'w', encoding='utf-8') as f:
            json.dump(results_data, f, indent=2, ensure_ascii=False)
        
        # CSV export
        csv_file = self.output_dir / "skin_collection.csv"
        with open(csv_file, 'w', newline='', encoding='utf-8') as f:
            if skins_list:
                writer = csv.DictWriter(f, fieldnames=asdict(skins_list[0]).keys())
                writer.writeheader()
                for skin in skins_list:
                    writer.writerow(asdict(skin))
        
        # Summary report
        summary_file = self.output_dir / "scraping_summary.txt"
        with open(summary_file, 'w', encoding='utf-8') as f:
            f.write("RainmeterUI Complete Scraping Summary\n")
            f.write("=" * 50 + "\n\n")
            f.write(f"Scraping completed: {datetime.now()}\n")
            f.write(f"Total categories processed: {self.stats['categories_processed']}\n")
            f.write(f"Total pages scraped: {self.stats['pages_scraped']}\n")
            f.write(f"Total skins found: {self.stats['skins_found']}\n")
            f.write(f"Skins downloaded: {self.stats['skins_downloaded']}\n")
            f.write(f"Skins extracted: {self.stats['skins_extracted']}\n")
            f.write(f"Download failures: {self.stats['download_failures']}\n")
            f.write(f"Extraction failures: {self.stats['extraction_failures']}\n")
            f.write(f"Total size downloaded: {self.stats['total_size_downloaded'] / (1024*1024):.1f} MB\n")
            f.write(f"Errors encountered: {self.stats['errors']}\n\n")
            
            if self.failed_downloads:
                f.write("Failed Downloads:\n")
                for failed in self.failed_downloads:
                    f.write(f"  - {failed['skin']} ({failed['category']}): {failed['url']}\n")
        
        # Success/failure lists
        successful_skins = [s for s in skins_list if s.download_status == "extracted"]
        failed_skins = [s for s in skins_list if s.download_status in ["download_failed", "extraction_failed", "error"]]
        
        # Save successful skins list
        if successful_skins:
            success_file = self.output_dir / "successful_downloads.json"
            with open(success_file, 'w', encoding='utf-8') as f:
                json.dump([asdict(skin) for skin in successful_skins], f, indent=2, ensure_ascii=False)
        
        # Save failed skins list
        if failed_skins:
            failed_file = self.output_dir / "failed_downloads.json"
            with open(failed_file, 'w', encoding='utf-8') as f:
                json.dump([asdict(skin) for skin in failed_skins], f, indent=2, ensure_ascii=False)
        
        self.logger.info(f"Results saved to {self.output_dir}")
        self.logger.info(f"  - Complete collection: {results_file}")
        self.logger.info(f"  - CSV export: {csv_file}")
        self.logger.info(f"  - Summary: {summary_file}")
        
        if successful_skins:
            self.logger.info(f"  - Successful downloads: {len(successful_skins)} skins")
        if failed_skins:
            self.logger.info(f"  - Failed downloads: {len(failed_skins)} skins")
    
    def print_final_statistics(self):
        """Print comprehensive final statistics"""
        self.logger.info("\n" + "=" * 70)
        self.logger.info("COMPLETE SCRAPING STATISTICS")
        self.logger.info("=" * 70)
        self.logger.info(f"Categories processed: {self.stats['categories_processed']}")
        self.logger.info(f"Pages scraped: {self.stats['pages_scraped']}")
        self.logger.info(f"Total skins found: {self.stats['skins_found']}")
        self.logger.info(f"Skins successfully downloaded: {self.stats['skins_downloaded']}")
        self.logger.info(f"Skins successfully extracted: {self.stats['skins_extracted']}")
        self.logger.info(f"Download failures: {self.stats['download_failures']}")
        self.logger.info(f"Extraction failures: {self.stats['extraction_failures']}")
        
        if self.stats['total_size_downloaded'] > 0:
            size_mb = self.stats['total_size_downloaded'] / (1024 * 1024)
            self.logger.info(f"Total data downloaded: {size_mb:.1f} MB")
        
        self.logger.info(f"Total errors: {self.stats['errors']}")
        
        # Success rate
        if self.stats['skins_found'] > 0:
            download_rate = (self.stats['skins_downloaded'] / self.stats['skins_found']) * 100
            extraction_rate = (self.stats['skins_extracted'] / self.stats['skins_found']) * 100
            self.logger.info(f"Download success rate: {download_rate:.1f}%")
            self.logger.info(f"Extraction success rate: {extraction_rate:.1f}%")
        
        self.logger.info("=" * 70)
    
    def run_complete_scrape(self):
        """🚀 Run the complete AXIOM scraping pipeline with branding"""
        # Initialize AXIOM branding system
        branding = AXIOMBrandingSystem(
            enable_animation=PYGAME_AVAILABLE and NUMPY_AVAILABLE,
            enable_sound=PYGAME_AVAILABLE and NUMPY_AVAILABLE
        )
        
        try:
            # Show AXIOM startup sequence
            branding.show_startup_sequence()
            
            self.logger.info("🤖 AXIOM Complete Scraping Pipeline Initiated")
            self.logger.info("=" * 70)
            self.logger.info("Pipeline: Extract Links → Download Packages → Extract Assets")
            self.logger.info("=" * 70)
            
            # Get categories to process
            categories_to_scrape = []
            
            if 'primary_skin_categories' in self.config:
                categories_to_scrape.extend(self.config['primary_skin_categories'])
            
            if 'additional_categories' in self.config:
                categories_to_scrape.extend(self.config['additional_categories'])
            
            total_categories = len(categories_to_scrape)
            self.logger.info(f"Processing {total_categories} categories")
            
            # Phase 1: Scrape all skin metadata
            all_scraped_skins = []
            
            for idx, category in enumerate(categories_to_scrape, 1):
                self.logger.info(f"\n--- PHASE 1: Scraping Category {idx}/{total_categories}: {category.get('name')} ---")
                
                try:
                    category_skins = self.scrape_category_complete(category)
                    all_scraped_skins.extend(category_skins)
                    self.stats['categories_processed'] += 1
                    
                    # Save progress after each category
                    self.all_skins.update(category_skins)
                    self.save_progress()
                    
                except Exception as e:
                    self.logger.error(f"Failed to scrape category {category.get('name')}: {e}")
                    self.stats['errors'] += 1
            
            # Update master collection
            self.all_skins.update(all_scraped_skins)
            
            self.logger.info(f"\n--- PHASE 1 COMPLETE: Found {len(self.all_skins)} total skins ---")
            
            # Phase 2: Download and extract all skins
            self.logger.info(f"\n--- PHASE 2: Downloading and Extracting Skins ---")
            
            skins_with_downloads = [s for s in self.all_skins if s.download_url]
            self.logger.info(f"Found {len(skins_with_downloads)} skins with download links")
            
            if skins_with_downloads:
                processed_skins = self.download_all_skins_parallel(list(self.all_skins))
                self.all_skins = set(processed_skins)
            
            # Phase 3: Save all results
            self.logger.info(f"\n--- PHASE 3: Saving Results ---")
            self.save_complete_results()
            self.save_progress()
            
            # Final statistics
            self.print_final_statistics()
            
            self.logger.info("\n🎉 COMPLETE SCRAPING PIPELINE FINISHED! 🎉")
            self.logger.info(f"Check '{self.output_dir}' for all downloaded skins and metadata")
            
        except KeyboardInterrupt:
            self.logger.info("\nScraping interrupted by user")
            self.save_progress()
            self.save_complete_results()
            
        except Exception as e:
            self.logger.error(f"Unexpected error during scraping: {e}")
            self.save_progress()
            raise


def main():
    """Main function with command line interface"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="RainmeterUI Complete Scraper - Extract, Download, and Extract Skins",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python rainmeter_complete_scraper.py
  python rainmeter_complete_scraper.py --output ./my_skins --delay 2.0
  python rainmeter_complete_scraper.py --workers 5 --delay 0.5
        """
    )
    
    parser.add_argument(
        '--config', 
        default='rainmeterui_categories.json',
        help='Path to categories configuration file (default: rainmeterui_categories.json)'
    )
    parser.add_argument(
        '--output', 
        default='scraped_data',
        help='Output directory for all scraped data (default: scraped_data)'
    )
    parser.add_argument(
        '--delay', 
        type=float, 
        default=1.0,
        help='Delay between requests in seconds (default: 1.0)'
    )
    parser.add_argument(
        '--workers',
        type=int,
        default=3,
        help='Number of parallel download workers (default: 3)'
    )
    
    args = parser.parse_args()
    
    print("🎯 RainmeterUI Complete Scraper Starting...")
    print(f"📁 Output directory: {args.output}")
    print(f"⏱️  Request delay: {args.delay}s")
    print(f"👥  Download workers: {args.workers}")
    print()
    
    # Create and run scraper
    scraper = RainmeterCompleteScraper(
        config_file=args.config,
        output_dir=args.output,
        delay=args.delay,
        max_workers=args.workers
    )
    
    scraper.run_complete_scrape()


if __name__ == "__main__":
    main()
