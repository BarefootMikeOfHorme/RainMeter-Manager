# RainmeterManager

> **Enterprise-grade live desktop management platform with dynamic content rendering, multi-backend support, and comprehensive security hardening**

[![Version](https://img.shields.io/badge/version-1.0.0--beta1-blue.svg)](https://github.com/BarefootMikeOfHorme/RainMeter-Manager/releases/tag/v1.0.0-beta1)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-lightgrey.svg)](https://www.microsoft.com/windows)
[![License](https://img.shields.io/badge/license-Proprietary-red.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/BarefootMikeOfHorme/RainMeter-Manager)

---

## 🎯 Overview

**RainmeterManager** is a next-generation Windows desktop management platform that transforms static desktops into dynamic, data-driven environments. Built with enterprise-grade architecture, the system provides real-time content aggregation, multi-backend rendering (SkiaSharp, Direct3D, WebView2), and secure inter-process communication between native C++ core and managed .NET render processes.

The platform enables users to create live, responsive desktop experiences with integrated data from 50+ curated sources including NASA space imagery, real-time weather, financial markets, news feeds, scientific data, and artistic content—all managed through a sophisticated widget-based framework with comprehensive security controls.

---

## 🚀 Key Features

### Content Ecosystem
- **50+ Curated Content Sources**: Pre-integrated APIs for space (NASA APOD, Mars rovers, EPIC Earth), weather (OpenWeatherMap, NOAA, WeatherAPI), news (Reddit, Hacker News), finance (crypto, forex), art (Met Museum, Rijksmuseum), and science (USGS earthquakes, arXiv papers)
- **Smart Content Loaders**: FileContentLoader (50+ formats with syntax highlighting), WebContentLoader (REST APIs with caching), MediaContentLoader (video/audio metadata), APIContentLoader (dynamic environments)
- **API Key Management**: Interactive wizard with automatic testing, secure credential storage, and environment variable configuration
- **Integrity Verification**: SHA256 hash validation for all deployed components

### Rendering Architecture
- **Multi-Backend Support**: 
  - **SkiaSharp**: High-performance 2D graphics with GPU acceleration
  - **Direct3D**: Native DirectX rendering for maximum performance
  - **WebView2**: Modern web content rendering with Chromium engine
- **Widget Framework**: Extensible component system for custom desktop elements
- **Real-time Updates**: Live data refresh with configurable intervals (5 seconds to 24 hours)
- **Dynamic Environments**: Time-based themes, weather-reactive backgrounds, animated effects

### Security & Reliability
- **Defense-in-Depth**: Buffer security checks (/GS), SDL checks (/sdl), ASLR, DEP, Control Flow Guard
- **Cryptographic Services**: AES-256-GCM encryption with BCrypt, secure key derivation, CSPRNG
- **Audit Trail**: Comprehensive logging with severity levels, async I/O, rotation policies
- **Stack Trace Symbolization**: DbgHelp integration for crash analysis and debugging
- **Code Signing Ready**: Full documentation for EV certificate acquisition and integration

### Developer Experience
- **CMake Build System**: Cross-configuration support (Debug/Release), parallel compilation, security flags
- **Comprehensive Testing**: 15+ unit tests for core components, integration test framework
- **IPC Bridge**: High-performance shared memory and named pipe communication
- **Service Locator Pattern**: Dependency injection for testability and modularity
- **NSIS Installer**: Automated prerequisite detection (VC++ 2015-2022, WebView2), proper uninstall

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     RainmeterManager (C++)                      │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐   │
│  │   Security   │  │    Logger    │  │  Configuration     │   │
│  │   (AES-GCM)  │  │  (Async I/O) │  │     Manager        │   │
│  └──────────────┘  └──────────────┘  └────────────────────┘   │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐   │
│  │ Widget Mgr   │  │ System Mon   │  │  Telemetry Svc     │   │
│  │ (Framework)  │  │ (Perf/CPU)   │  │  (Metrics)         │   │
│  └──────────────┘  └──────────────┘  └────────────────────┘   │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │              IPC Bridge (Shared Memory + Pipes)           │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    RenderProcess (.NET 8.0)                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌────────────────┐ │
│  │  SkiaSharp      │  │   Direct3D      │  │   WebView2     │ │
│  │  Renderer       │  │   Renderer      │  │   Renderer     │ │
│  └─────────────────┘  └─────────────────┘  └────────────────┘ │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                    Content Loaders                        │ │
│  │  • FileContentLoader    • WebContentLoader                │ │
│  │  • MediaContentLoader   • APIContentLoader                │ │
│  └───────────────────────────────────────────────────────────┘ │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │    IPC Message Handler + Performance Monitor              │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🛠️ Technical Stack

### Core Application (Native)
- **Language**: C++17
- **Build System**: CMake 3.20+
- **Platform**: Windows 10/11 (x64)
- **Dependencies**: 
  - Windows SDK 10.0.22621.0
  - Visual C++ 2015-2022 Redistributable
  - BCrypt (Cryptography)
  - DbgHelp (Crash Analysis)

### Render Process (Managed)
- **Framework**: .NET 8.0
- **UI Libraries**: 
  - SkiaSharp 2.88.8 (2D Graphics)
  - Microsoft.Web.WebView2 1.0.2839.39 (Web Content)
  - SharpDX (Direct3D Interop)
- **HTTP**: System.Net.Http with HttpClient pooling
- **Serialization**: System.Text.Json

### Testing & Quality
- **Unit Tests**: Google Test (GTest)
- **Coverage**: OpenCppCoverage (70%+ target)
- **Static Analysis**: Visual Studio Code Analysis, Cppcheck
- **Security Scanning**: Gitleaks, TruffleHog

---

## 📦 Installation

### Prerequisites
- Windows 10 (1809+) or Windows 11
- Visual Studio 2022 with C++ development tools
- .NET 8.0 SDK
- CMake 3.20 or higher
- Git 2.30+

### Quick Start

```powershell
# Clone repository
git clone https://github.com/BarefootMikeOfHorme/RainMeter-Manager.git
cd RainMeter-Manager

# Configure build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release -j 4

# Run
.\build\Release\RainmeterManager.exe
```

### Content Loader Setup

```powershell
# Navigate to content loader directory
cd renderprocess\Content

# Run interactive API key setup wizard
.\setup_api_keys.ps1

# Deploy content loaders
.\deploy_loaders.ps1 -Force

# Verify installation
.\verify_loaders.ps1
```

See [renderprocess/Content/README.md](renderprocess/Content/README.md) for detailed setup instructions.

---

## 📊 Current Status

### ✅ Completed Components (Phase 0-3)
- [x] Core architecture with service locator pattern
- [x] Security subsystem (AES-256-GCM, BCrypt)
- [x] Asynchronous logging with rotation
- [x] Widget management framework
- [x] Configuration manager with validation
- [x] IPC bridge (shared memory + named pipes)
- [x] Multi-backend rendering system (SkiaSharp, Direct3D, WebView2)
- [x] 50+ curated content sources
- [x] API key management suite
- [x] Enterprise deployment tools
- [x] Security hardening (ASLR, DEP, CFG, /GS, /SDL)
- [x] 15 logger unit tests
- [x] Security audit documentation
- [x] NSIS installer with prerequisites

### 🚧 In Progress (Phase 4-5)
- [ ] Widget UI manager integration
- [ ] Dashboard service with live tiles
- [ ] Process lifecycle management
- [ ] Performance profiling dashboard
- [ ] Real-time telemetry aggregation

### 📅 Planned (Phase 6-8)
- [ ] Plugin SDK for third-party widgets
- [ ] Cloud sync for configurations
- [ ] Multi-monitor optimization
- [ ] Mobile companion app
- [ ] Community widget marketplace

**Release Target**: Q2 2025 (v1.0.0)

---

## 📖 Documentation

### User Guides
- [Installation Guide](docs/INSTALLATION.md)
- [API Key Setup](renderprocess/Content/API_KEY_GUIDE.md)
- [Content Loader Guide](renderprocess/Content/README.md)
- [Widget Configuration](docs/WIDGET_CONFIGURATION.md)

### Developer Documentation
- [Architecture Overview](docs/ARCHITECTURE.md)
- [Build Instructions](docs/BUILD.md)
- [Security Audit Report](docs/SECURITY_AUDIT.md)
- [Code Signing Process](docs/CODE_SIGNING_PROCESS.md)
- [Release Compliance Checklist](RELEASE_COMPLIANCE_CHECKLIST.md)

### API Reference
- [C++ Core API](docs/api/CORE_API.md)
- [RenderProcess API](docs/api/RENDER_API.md)
- [IPC Protocol Specification](docs/api/IPC_PROTOCOL.md)
- [Widget Framework API](docs/api/WIDGET_API.md)

---

## 🎯 Use Cases

### Personal Desktop Enhancement
Transform your desktop into a personalized command center with live weather, space imagery, system monitoring, news feeds, and cryptocurrency tracking—all elegantly integrated and customizable.

### Development Workstation
Monitor build pipelines, GitHub activity, system performance, and project metrics directly on your desktop background without switching context from your IDE.

### Data Visualization
Create custom data dashboards pulling from REST APIs, databases, or file systems. Display real-time charts, graphs, and metrics with interactive WebView2-powered visualizations.

### Digital Signage
Deploy RainmeterManager on kiosk systems or public displays for dynamic information boards, weather stations, transportation schedules, or corporate dashboards.

### Education & Research
Display live scientific data (earthquakes, astronomy, weather patterns), research paper feeds (arXiv), and educational content directly integrated with your workspace.

---

## 🔒 Security

RainmeterManager implements defense-in-depth security principles:

### Compile-Time Protections
- Buffer Security Checks (`/GS`)
- Security Development Lifecycle Checks (`/sdl`)
- Control Flow Guard (`/GUARD:CF`)
- Safe Exception Handlers (`/SAFESEH` for 32-bit)

### Runtime Protections
- Address Space Layout Randomization (ASLR) with high entropy (`/HIGHENTROPYVA`)
- Data Execution Prevention (DEP/NX) (`/NXCOMPAT`)
- AES-256-GCM authenticated encryption for sensitive data
- Secure credential storage integration (Windows Credential Manager)

### Operational Security
- Comprehensive audit logging with security event tracking
- API key isolation via environment variables
- Code signing ready (EV certificate process documented)
- Regular dependency updates and vulnerability scanning

**Security Audit**: [docs/SECURITY_AUDIT.md](docs/SECURITY_AUDIT.md)

---

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Workflow
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-widget`)
3. Commit your changes (`git commit -m 'Add amazing widget'`)
4. Push to the branch (`git push origin feature/amazing-widget`)
5. Open a Pull Request

### Code Standards
- Follow existing code style (see `.clang-format` and `.editorconfig`)
- Write unit tests for new features
- Update documentation for API changes
- Run static analysis before submitting

---

## 📄 License

Copyright © 2025 RainmeterManager Project

This project is proprietary software. All rights reserved. Unauthorized copying, modification, distribution, or use of this software, via any medium, is strictly prohibited without explicit written permission from the copyright holder.

For licensing inquiries, please contact: [license@rainmetermanager.dev](mailto:license@rainmetermanager.dev)

---

## 🙏 Acknowledgments

### Technologies
- **SkiaSharp**: Cross-platform 2D graphics by Mono Project
- **Google Test**: C++ testing framework
- **CMake**: Cross-platform build system
- **WebView2**: Microsoft Edge WebView2 runtime
- **NASA Open APIs**: Space imagery and data
- **NOAA**: Weather and environmental data
- **OpenStreetMap**: Mapping data

### Data Sources
Special thanks to the providers of free, public APIs that make this project's content ecosystem possible: NASA, NOAA, USGS, OpenWeatherMap, SpaceX Data, Reddit, Hacker News, Wikipedia, Wikimedia Commons, and many others.

---

## 📞 Support

### Community
- **Issues**: [GitHub Issues](https://github.com/BarefootMikeOfHorme/RainMeter-Manager/issues)
- **Discussions**: [GitHub Discussions](https://github.com/BarefootMikeOfHorme/RainMeter-Manager/discussions)
- **Wiki**: [GitHub Wiki](https://github.com/BarefootMikeOfHorme/RainMeter-Manager/wiki)

### Commercial Support
For enterprise support, custom development, or consulting services:
- Email: [support@rainmetermanager.dev](mailto:support@rainmetermanager.dev)
- Website: [www.rainmetermanager.dev](https://www.rainmetermanager.dev)

---

## 🗺️ Roadmap

### Q1 2025 - Beta 1 (Current)
- ✅ Core platform stability
- ✅ Content loader ecosystem (50+ sources)
- ✅ Security hardening
- ✅ API key management
- ✅ Comprehensive documentation

### Q2 2025 - v1.0.0 Release
- Dashboard service completion
- Widget UI manager polish
- Performance optimization
- Code signing integration
- Community widget examples

### Q3 2025 - v1.1.0
- Plugin SDK release
- Cloud sync (OneDrive, Google Drive)
- Mobile companion app (Android/iOS)
- Advanced customization UI

### Q4 2025 - v1.2.0
- Widget marketplace
- Multi-monitor enhancements
- AI-powered widget suggestions
- Advanced analytics dashboard

---

## 📈 Project Statistics

- **Lines of Code**: ~45,000+ (C++ Core: 20K, .NET Render: 15K, Scripts/Docs: 10K)
- **Components**: 25+ core services and managers
- **Content Sources**: 50+ pre-integrated APIs
- **Test Coverage**: 70%+ (target)
- **Documentation**: 15+ comprehensive guides
- **Supported Formats**: 50+ file types (FileContentLoader)
- **Active Development**: Since September 2024

---

<div align="center">

**Built with ❤️ for the Windows Desktop Community**

[⬆ Back to Top](#rainmetermanager)

</div>
