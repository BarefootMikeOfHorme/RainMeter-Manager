# RainmeterManager Stubs and Placeholders

This document tracks stub files, placeholder implementations, and TODOs throughout the codebase.

**Created:** 2025-10-18  
**Purpose:** Document all stub/placeholder code that needs real implementation

---

## ✅ Completed Stubs

### Widget Framework (`src/widgets/framework/`)
- ✅ **widget_base.h** - Base classes for all widgets
  - `IWidget` interface
  - `WidgetBase` implementation
  - `WidgetConfig` structure
  - `WidgetFactory` for creating widgets
  - Ready for widget implementations (CPUHexagon, 3DOrbs, NetworkGlobe, etc.)

### System Monitoring (`src/core/`)
- ✅ **system_monitor.h** - System monitoring interface
  - `ISystemMonitor` interface
  - `WindowsSystemMonitor` stub (needs native Windows API implementation)
  - `PythonServiceMonitor` stub (for Python service integration)
  - Data structures: `CPUInfo`, `MemoryInfo`, `ProcessInfo`, `NetworkInterfaceInfo`, `DiskInfo`

### Test Infrastructure (`tests/unit/`)
- ✅ **logger_tests.cpp** - GoogleTest stubs for Logger
  - 14 test cases defined with `GTEST_SKIP()`
  - Remove `GTEST_SKIP()` and implement each test

---

## 🔄 In Progress / TODO

### Test Stubs (PRIORITY: CRITICAL)
Location: `tests/unit/`

#### TODO: Create security_tests.cpp
```cpp
// Test cases needed:
- [ ] BCrypt provider initialization
- [ ] AES-GCM encryption/decryption
- [ ] DPAPI integration
- [ ] Code signature validation
- [ ] Hash calculation (SHA-256)
- [ ] Thread safety of crypto operations
```

#### TODO: Create service_locator_tests.cpp
```cpp
// Test cases needed:
- [ ] Service registration
- [ ] Service retrieval
- [ ] Service lifecycle
- [ ] Thread-safe access
- [ ] Service not found handling
```

#### TODO: Create config_manager_tests.cpp
```cpp
// Test cases needed:
- [ ] Load configuration
- [ ] Save configuration  
- [ ] Configuration validation
- [ ] Default values
- [ ] Configuration migration
```

### Widget Implementations
Location: `src/widgets/`

#### TODO: CPUHexagonWidget
Based on `suggested widgets or ideas/cpuhexagon.txt`
```cpp
class CPUHexagonWidget : public WidgetBase {
    // TODO: Implement hexagonal CPU core display
    // - Render hexagon grid
    // - Map CPU cores to hexagons
    // - Color based on load
    // - Mouse interaction for process info
};
```

#### TODO: 3DOrbsWidget
Based on `suggested widgets or ideas/3d orbs.txt`
```cpp
class ThreeDOrbsWidget : public WidgetBase {
    // TODO: Implement 3D orbs for memory/processes
    // - Central orb for memory
    // - Orbiting process indicators
    // - Size based on memory usage
    // - Click to kill process
};
```

#### TODO: NetworkGlobeWidget
Based on `suggested widgets or ideas/3d global network.txt`
```cpp
class NetworkGlobeWidget : public WidgetBase {
    // TODO: Implement rotating globe with connections
    // - 3D globe rendering
    // - Network connection lines
    // - Geolocation display
};
```

### System Monitor Implementation
Location: `src/core/system_monitor.cpp` (needs to be created)

#### TODO: WindowsSystemMonitor Implementation
```cpp
// Implement using native Windows APIs:
- [ ] PDH (Performance Data Helper) for CPU/memory
- [ ] WMI for process information
- [ ] GetAdaptersInfo for network
- [ ] GetDiskFreeSpaceEx for disk info
- [ ] Performance counters
```

#### TODO: PythonServiceMonitor Implementation
```cpp
// Implement HTTP client to talk to manager_service.py:
- [ ] HTTP GET to http://localhost:8000/data
- [ ] Parse JSON response
- [ ] Map to C++ data structures
- [ ] Handle service not running
- [ ] HTTP POST for kill process
```

### Service Registration
Location: `src/app/main.cpp` (lines 300-312)

Currently marked as PLACEHOLDER:
```cpp
LOG_INFO("Registering core services...");
// TODO: Implement service locator pattern
// - SecurityManager
// - TelemetryService  
// - ConfigurationManager
// - UIFramework
// - WidgetManager
LOG_INFO("Service registration: PLACEHOLDER");
```

#### TODO: Implement Service Interfaces
- [ ] Create `ConfigurationManager` class
- [ ] Create `TelemetryService` class  
- [ ] Create `UIFramework` class
- [ ] Create `WidgetManager` class
- [ ] Register with ServiceLocator

### IPC and Render Process
Location: `src/app/rainmgrapp.cpp` (lines 610-616)

Currently commented out:
```cpp
// TODO: Uncomment when IPCManager is implemented
// IPCManager::Instance().Initialize();

// TODO: Uncomment when RenderProcess launcher is implemented
// LaunchRenderProcess();
```

#### TODO: IPC Manager
```cpp
class IPCManager {
    // TODO: Implement inter-process communication
    // - Named pipes for Windows
    // - Message passing protocol
    // - Handle multiple render processes
};
```

#### TODO: RenderProcess Launcher
```cpp
// TODO: Implement render process spawning
// - Launch separate process for rendering
// - Pass IPC channel info
// - Monitor process health
// - Restart on crash
```

### Installer
Location: `installer/installer.nsi.txt`

Currently just a text file. Needs conversion to working NSIS script.

#### TODO: NSIS Installer Script
```nsis
; TODO: Implement full installer
- [ ] Component selection
- [ ] File installation
- [ ] Registry entries
- [ ] Uninstaller creation
- [ ] Start menu shortcuts
- [ ] Optional: Check for prerequisites
```

---

## 📊 Implementation Priority

### Phase 1: Critical (Weeks 1-2)
1. ✅ Test framework stubs (logger_tests.cpp done)
2. ⏳ security_tests.cpp
3. ⏳ service_locator_tests.cpp
4. ⏳ config_manager_tests.cpp
5. ⏳ WindowsSystemMonitor implementation

### Phase 2: High (Weeks 3-4)
6. ⏳ Service interfaces (ConfigurationManager, etc.)
7. ⏳ Service registration in main.cpp
8. ⏳ WidgetManager implementation
9. ⏳ One example widget (CPUHexagonWidget recommended)

### Phase 3: Medium (Weeks 5-6)
10. ⏳ IPC Manager implementation
11. ⏳ RenderProcess launcher
12. ⏳ Additional widgets (3DOrbs, NetworkGlobe)
13. ⏳ PythonServiceMonitor implementation

### Phase 4: Lower (Weeks 7+)
14. ⏳ NSIS installer
15. ⏳ Additional widget variants
16. ⏳ Performance optimizations
17. ⏳ Polish and refinement

---

## 🔧 How to Implement a Stub

### For Test Stubs:
1. Remove the `GTEST_SKIP()` line
2. Implement the actual test logic
3. Verify test passes
4. Mark as complete

### For Class Stubs:
1. Create corresponding `.cpp` file
2. Implement constructor/destructor
3. Implement each method with proper logic
4. Add error handling
5. Add logging
6. Write tests
7. Mark as complete

---

## 📝 Related Documents

- `CURRENT_STATE_ASSESSMENT.md` - Overall project status
- `RELEASE_COMPLIANCE_CHECKLIST.md` - Release requirements
- `changes/2025-09-03_Initial-Assessment/backlog.md` - Development backlog
- `suggested widgets or ideas/` - Widget implementation ideas

---

## 🎯 Next Steps

**When Copilot finishes the system monitoring code:**
1. Integrate with `WindowsSystemMonitor` class
2. Test data collection
3. Wire up to widget framework
4. Create first widget implementation

**Immediate Next:**
1. Create remaining test stubs (security, service_locator, config_manager)
2. Begin implementing tests one by one
3. Create service interface stubs
4. Document integration points

---

**Last Updated:** 2025-10-18  
**Status:** Initial stubs created, ready for implementation  
**Next Review:** After Copilot delivers system monitoring code
