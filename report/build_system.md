# Build System Analysis Report

## Primary Build Tools

### CMake Build System
- **Main CMake file**: `./CMakeLists.txt`
- **CMake version**: Minimum 3.14 required
- **Secondary CMake**: `./tests/CMakeLists.txt` (testing configuration)
- **Custom CMake modules**: `./cmake/FindSkiaSharp.cmake`
- **Version template**: `./cmake/version.h.in`

### Visual Studio Solution
- **Solution file**: `./RainmeterManager.sln` (Visual Studio 2017+ format)
- **Project file**: `./RainmeterManager.vcxproj`
- **Platform toolset**: v143 (Visual Studio 2022)
- **Target platforms**: Win32, x64
- **Configurations**: Debug, Release

## Language and Standards
- **Primary language**: C++ (C++17 standard)
- **Secondary language**: C (for some components)
- **Character set**: Unicode (UTF-16)

## Build Configuration

### Compiler Settings
#### MSVC (Primary)
- **Toolset**: v143 (Visual Studio 2022)
- **Warning level**: Level 4 (/W4)
- **Multi-processor compilation**: Enabled (/MP)
- **Exception handling**: Synchronous (/EHsc)
- **Runtime library**: 
  - Debug: Multi-threaded Debug (/MTd)
  - Release: Multi-threaded (/MT)

#### GCC/Clang (Alternative)
- **Warning flags**: -Wall -Wextra -Wpedantic
- **Debug sanitizers**: Address sanitizer support (optional)

### Output Directories
- **CMake outputs**: 
  - Runtime: `${CMAKE_BINARY_DIR}/bin`
  - Library: `${CMAKE_BINARY_DIR}/lib`
  - Archive: `${CMAKE_BINARY_DIR}/lib`
- **Visual Studio outputs**:
  - Binary: `build/bin/$(Platform)/$(Configuration)/`
  - Intermediate: `build/obj/$(Platform)/$(Configuration)/`

## External Dependencies

### Required System Libraries (Windows)
- `user32.lib` - Windows user interface
- `gdi32.lib` - Graphics Device Interface
- `comctl32.lib` - Common controls
- `shell32.lib` - Shell API
- `shlwapi.lib` - Shell lightweight API
- `comdlg32.lib` - Common dialog boxes
- `ole32.lib` - OLE automation
- `advapi32.lib` - Advanced Windows API
- `wininet.lib` - Windows internet API

### Optional Dependencies
- **SkiaSharp**: Graphics rendering library (optional)
  - Custom FindModule: `./cmake/FindSkiaSharp.cmake`
  - Supports NuGet package discovery
  - Falls back to native drawing methods if not found
  - Fallback preprocessor definition: `NO_SKIASHARP`

### Testing Framework
- **CTest**: Integrated testing (part of CMake)
- **Test structure**: Supports unit and integration test directories
- **Fallback**: Placeholder test if no actual tests are configured

## Build Features and Options

### CMake Options
- `BUILD_TESTS`: Enable test suite building (ON by default)
- `BUILD_INSTALLER`: Enable installer package (ON by default)  
- `ENABLE_LOGGING`: Enable logging functionality (ON by default)
- `ENABLE_SANITIZERS`: Enable address sanitizers in debug (OFF by default)

### Version Management
- Automatic version extraction from `src/version.h`
- Template-based version generation
- Default version: 1.0.0.0

## Project Structure Support
- **Source organization**: 
  - Core functionality: `src/core/`
  - UI components: `src/ui/`
  - Widget system: `src/widgets/`
- **Resource management**: `resources/` directory
- **IDE integration**: Source groups for Visual Studio
