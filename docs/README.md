# Rainmeter Manager

A comprehensive management tool for Rainmeter skins with enterprise-grade security features, designed to make it easy to browse, edit, and package Rainmeter skins safely and securely.

## 🔒 **Phase 1 Complete - Security Framework**

**Enterprise Security Features:**
- ✅ **AES-GCM Encryption** - Secure data encryption using Windows BCrypt
- ✅ **Code Signature Verification** - WinTrust API validation of executables
- ✅ **Windows DPAPI** - Secure API key storage
- ✅ **Malware Scanning** - Pattern-based malicious content detection
- ✅ **Domain Whitelisting** - Restricted network access for security

## Features

- Browse and install Rainmeter skins with security validation
- Edit skin configuration files with syntax highlighting
- Create .rmskin packages for distribution
- **Enhanced Security**: Code signing verification and malware scanning
- **Secure API Management**: Encrypted storage of API keys
- Automatic dependency management (Rainmeter, Scintilla, Visual C++ Redistributable)
- Folder browsing and file management with security checks
- Support for various background modes

## System Requirements

- Windows 10 or later (64-bit)
- 50MB of free disk space
- Rainmeter 4.5 or later (automatically installed if needed)

## Building from Source

### Prerequisites

**Required Dependencies (Phase 1 Security Framework):**
- Visual Studio 2022 or later with C++ Desktop Development workload
- Windows 10 Build 19041+ (for security libraries: bcrypt.dll, wintrust.dll, crypt32.dll)
- NSIS (Nullsoft Scriptable Install System) for creating the installer
- Git (optional, for version control)

**Verify Dependencies:**
```powershell
# Check all required dependencies
.\scripts\verify_dependencies.ps1

# List dependencies without checking
.\scripts\verify_dependencies.ps1 -ListOnly
```

### Build Instructions

1. Clone the repository or download the source code
   ```
   git clone https://github.com/yourname/rainmeter-manager.git
   cd rainmeter-manager
   ```

2. Run the build script from a Visual Studio Developer Command Prompt
   ```
   build.bat
   ```

3. The executable and installer will be created in the `build` directory

### Project Structure

- `src/` - Source code files
- `res/` - Resource files (icons, images, etc.)
- `installer/` - Installer scripts and resources
- `build/` - Build output directory (created during build)

## Deployment

The application can be deployed in several ways:

1. **Installer Package (Recommended)**
   - Run the NSIS installer `RainmeterManager-Setup.exe`
   - This will install the application and all required dependencies

2. **Manual Installation**
   - Copy the `RainmeterManager.exe` and required DLLs to a directory
   - Ensure Visual C++ Redistributable 2022 is installed
   - Ensure Rainmeter is installed
   - Ensure Scintilla is available (SciLexer.dll must be in the same directory)

## Configuration

The application automatically detects Rainmeter installation paths. If Rainmeter is not found, it will offer to download and install it. Configuration settings are stored in:

- `%APPDATA%\RainmeterManager\settings.ini` - User preferences
- `%LOCALAPPDATA%\RainmeterManager\cache` - Local cache for downloaded content

## Usage

1. **Managing Widgets**
   - Browse and select widgets from the list on the left
   - Click "Edit Widget" to modify configuration
   - Click "Install Widget" to activate in Rainmeter

2. **Managing Dashboards**
   - Browse dashboards in the right list
   - Double-click to load an entire dashboard
   - Right-click for additional options

3. **Creating Packages**
   - Select a widget or dashboard
   - Click "Create Package"
   - Follow the on-screen instructions to create a .rmskin package

4. **Folder Management**
   - Click the folder icon to browse your documents
   - Drag and drop files to import

## Self-Update Mechanism

The application checks for updates on startup. If a new version is available, it will prompt to download and install it automatically.

## Security Configuration

**Phase 1 Security Features** are automatically configured, but you can customize:

- **API Providers**: Edit `config/api_providers.json` for external data source settings
- **Dependencies**: Review `dependencies.json` for security library requirements
- **Domain Whitelist**: Configure allowed domains for network requests

**Security Files:**
- `config/api_providers.json` - API configuration with rate limits and security settings
- `dependencies.json` - Complete dependency specification
- `docs/guides/LEGAL_AND_CONFIG.md` - Legal compliance and configuration guide

## API Documentation

**Developer Resources:**
- `docs/api/DEVELOPER_API.md` - Complete API reference with Phase 1 security features
- `docs/guides/INTEGRATION_GUIDE.md` - Custom data sources and widget development
- `design_docs/bootstrap.md` - Application architecture and bootstrap design

**Phase 1 Security API Example:**
```cpp
#include "src/core/security.h"

// Secure API key storage
SecurityManager security;
security.storeAPIKeySecure("openweathermap", "your_api_key");
std::string key = security.getAPIKeySecure("openweathermap");

// File integrity verification
if (!security.verifyCodeSignature("plugin.dll")) {
    LOG_ERROR("Plugin failed signature verification");
}
```

## Troubleshooting

If you encounter issues:

1. **Dependency Issues**: Run `scripts\verify_dependencies.ps1` to check all requirements
2. Check the log file at `%APPDATA%\RainmeterManager\RainmeterManager.log`
3. Ensure all dependencies are installed correctly (especially Phase 1 security libraries)
4. Try reinstalling the application

## License

This project is licensed under the MIT License - see the LICENSE.txt file for details.

## Acknowledgements

- Rainmeter team for their amazing application
- Scintilla project for the text editor component
- All contributors to this project