# Rainmeter Widget Catalog - Comprehensive Analysis
**Analysis Date:** October 18, 2025  
**Source:** 962 INI files, 96 Lua scripts, 3,492 assets from 31 skin suites  
**Purpose:** Extract all patterns for RainmeterManager C++ widget system

---

## Executive Summary

**Collection Stats:**
- **962 widget configuration files** (.ini)
- **96 Lua scripts** (advanced logic/data processing)
- **3,492 images** (backgrounds, icons, meters, overlays)
- **18 native plugins** (AudioLevel, CoreTemp, HWiNFO, NowPlaying, etc.)
- **31 complete skin suites** (Evolucion, NXT-OS, Dashboard, BlueVision, etc.)

---

## 1. MEASURE TYPES (Data Sources)

### 1.1 System Performance Measures

#### CPU Monitoring
```ini
[MeasureCPU]
Measure=CPU
Processor=0              ; 0 = average, 1-N = specific core
MinValue=0
MaxValue=100
```

**Patterns Found:**
- Per-core monitoring (Processor=1, 2, 3, etc.)
- Average CPU load (Processor=0)
- Used with `UpdateDivider` for efficiency
- Often paired with Max/Min value tracking

#### Memory Monitoring
```ini
[MeasureRAM]
Measure=PhysicalMemory
UpdateDivider=20         ; Update every 20 seconds
```

```ini
[MeasureSWAP]
Measure=SwapMemory
UpdateDivider=20
```

**Patterns:**
- Physical memory (RAM)
- Swap memory (page file)
- Auto-scaling with `AutoScale=1`
- Percentual display with `Percentual=1`

#### Disk Monitoring
```ini
[MeasureDiskFree]
Measure=FreeDiskSpace
Drive=C:
UpdateDivider=60

[MeasureDiskTotal]
Measure=FreeDiskSpace
Drive=C:
Total=1

[MeasureDiskIO]
Measure=Plugin
Plugin=UsageMonitor
Category=LogicalDisk
Counter=% Disk Time
Instance=C:
```

**Patterns:**
- Free/used/total disk space
- Disk I/O read/write rates
- Multiple drive monitoring
- SMART data via plugins

### 1.2 Network Measures

```ini
[MeasureNetIn]
Measure=NetIn
Interface=0              ; 0 = default, Best = auto-select
NetInSpeed=10485760     ; Max download in bytes/sec

[MeasureNetOut]
Measure=NetOut
Interface=0
NetOutSpeed=10485760

[MeasureNetTotal]
Measure=NetIn
Cumulative=1            ; Total bytes received

[MeasureWAN]
Measure=Plugin
Plugin=WebParser
Url=http://checkip.dyndns.org
RegExp="(?siU)Address: (.*)</body>"
StringIndex=1

[MeasureLAN]
Measure=Plugin
Plugin=SysInfo
SysInfoType=IP_ADDRESS
SysInfoData=0
```

**Patterns:**
- Upload/download speed
- Cumulative bandwidth tracking
- External IP via WebParser
- Local IP via SysInfo
- Interface selection (Ethernet, WiFi)
- Ping monitoring via PingPlugin

### 1.3 Hardware Sensors (via Plugins)

#### GPU Monitoring (HWiNFO Plugin)
```ini
[MeasureGPUName]
Measure=Plugin
Plugin=HWiNFO
HWiNFOID=9007000
HWiNFOType=SensorName

[MeasureGPULoad]
Measure=Plugin
Plugin=HWiNFO
HWiNFOID=9007000
HWiNFOType=Value
MinValue=0
MaxValue=100

[MeasureGPUTemp]
Measure=Plugin
Plugin=HWiNFO
HWiNFOID=9001003
HWiNFOType=Value
MinValue=0
MaxValue=100

[MeasureGPUClock]
Measure=Plugin
Plugin=HWiNFO
HWiNFOID=11006001
HWiNFOType=Value
```

#### CPU Temperature (CoreTemp Plugin)
```ini
[MeasureCoreTemp]
Measure=Plugin
Plugin=CoreTemp
CoreTempType=Temperature
CoreTempIndex=0         ; 0 = average, 1-N = specific core
```

**HWiNFO IDs Discovered:**
- GPU Core Load: 9007000
- GPU Temperature: 9001003
- GPU Clock Speed: 11006001
- CPU Package Temp: varies by system
- Motherboard sensors: varies
- Fan speeds: varies

### 1.4 Media Player Measures

```ini
[MeasurePlayer]
Measure=Plugin
Plugin=NowPlaying
PlayerName=Winamp       ; Winamp, WMP, iTunes, foobar2000, Spotify, etc.
PlayerType=TITLE

[MeasureArtist]
Measure=Plugin
Plugin=NowPlaying
PlayerName=[MeasurePlayer]
PlayerType=ARTIST

[MeasureAlbum]
Measure=Plugin
Plugin=NowPlaying
PlayerName=[MeasurePlayer]
PlayerType=ALBUM

[MeasureCover]
Measure=Plugin
Plugin=NowPlaying
PlayerName=[MeasurePlayer]
PlayerType=COVER
Substitute="":"#@#Images\Default.png"

[MeasureProgress]
Measure=Plugin
Plugin=NowPlaying
PlayerName=[MeasurePlayer]
PlayerType=PROGRESS

[MeasureState]
Measure=Plugin
Plugin=NowPlaying
PlayerName=[MeasurePlayer]
PlayerType=STATE        ; 0=stopped, 1=playing, 2=paused
```

**Supported Players:**
- Winamp, Windows Media Player, iTunes, foobar2000
- Spotify, MusicBee, MediaMonkey, AIMP
- Custom players via plugin API

### 1.5 Audio Visualization Measures

```ini
[MeasureAudio]
Measure=Plugin
Plugin=AudioLevel
Port=Output             ; Output or Input
FFTSize=2048           ; 1024, 2048, 4096, 8192, 16384
FFTOverlap=1024
FFTAttack=15
FFTDecay=200
Bands=50               ; Number of frequency bands
FreqMin=20
FreqMax=16000

[MeasureBand0]
Measure=Plugin
Plugin=AudioLevel
Parent=MeasureAudio
Type=Band
BandIdx=0
```

**Patterns:**
- FFT-based spectrum analysis
- Configurable frequency bands
- Attack/decay smoothing
- Parent/child measure hierarchy
- Line visualizers, bar visualizers, circular spectrum

### 1.6 Weather Data (WebParser)

```ini
[MeasureWeather]
Measure=Plugin
Plugin=WebParser
UpdateRate=1800         ; Update every 30 minutes
Url=http://wxdata.weather.com/wxdata/weather/local/#Location#?cc=*&unit=#Unit#&dayf=8
RegExp="(?siU)<weather ver=\"(.*)\">(.*)<tmp>(.*)</tmp>(.*)<t>(.*)</t>(.*)<icon>(.*)</icon>"
StringIndex=1

[MeasureCurrentTemp]
Measure=Plugin
Plugin=WebParser
Url=[MeasureWeather]
StringIndex=3

[MeasureCondition]
Measure=Plugin
Plugin=WebParser
Url=[MeasureWeather]
StringIndex=5
Substitute=#WeatherCodes#

[MeasureIcon]
Measure=Plugin
Plugin=WebParser
Url=[MeasureWeather]
StringIndex=7
```

**Weather APIs Found:**
- Weather.com (wxdata.weather.com)
- OpenWeatherMap
- Weather Underground
- NOAA/Weather.gov
- Local weather stations

### 1.7 Time & Date Measures

```ini
[MeasureTime]
Measure=Time
Format=%H:%M           ; 24-hour: HH:MM
; Format=%I:%M %p    ; 12-hour: HH:MM AM/PM

[MeasureDate]
Measure=Time
Format=%d.%m.%Y        ; DD.MM.YYYY

[MeasureDay]
Measure=Time
Format=%A              ; Full day name

[MeasureMonth]
Measure=Time
Format=%B              ; Full month name
```

### 1.8 System Information

```ini
[MeasureComputerName]
Measure=Plugin
Plugin=SysInfo
SysInfoType=COMPUTER_NAME

[MeasureUserName]
Measure=Plugin
Plugin=SysInfo
SysInfoType=USER_NAME

[MeasureOS]
Measure=Plugin
Plugin=SysInfo
SysInfoType=OS_VERSION

[MeasureUptime]
Measure=Uptime
Format=%4!i! Days, %3!i!:%2!02i!

[MeasureCPUName]
Measure=Registry
RegHKey=HKEY_LOCAL_MACHINE
RegKey=HARDWARE\DESCRIPTION\System\CentralProcessor\0
RegValue=ProcessorNameString
Substitute="  ":""
```

### 1.9 Power & Battery

```ini
[MeasureBattery]
Measure=Plugin
Plugin=PowerPlugin
PowerState=PERCENT

[MeasureBatteryLifetime]
Measure=Plugin
Plugin=PowerPlugin
PowerState=LIFETIME

[MeasureACStatus]
Measure=Plugin
Plugin=PowerPlugin
PowerState=ACLINE       ; 0=battery, 1=AC

[MeasureCPUFrequency]
Measure=Plugin
Plugin=PowerPlugin
PowerState=MHZ
```

### 1.10 Advanced: Lua Scripting

```lua
function Initialize()
    -- Initialize variables and state
    measureCPU = SKIN:GetMeasure('MeasureCPU')
    measureGPU = SKIN:GetMeasure('MeasureGPU')
    historyData = {}
    for i=1,900 do
        historyData[i] = 0
    end
end

function Update()
    -- Called every update cycle
    -- Shift history array
    for i=1,899 do
        historyData[i] = historyData[i+1]
    end
    historyData[900] = measureCPU:GetValue()
    
    -- Return value to Rainmeter
    return historyData[900]
end

function GetMaxValue()
    local max = 0
    for i=1,900 do
        if historyData[i] > max then
            max = historyData[i]
        end
    end
    return max
end
```

**Lua Patterns:**
- Data smoothing and filtering
- Historical data tracking
- Complex calculations
- Dynamic meter positioning
- Custom visualizations
- String manipulation
- API calls and data parsing

---

## 2. METER TYPES (Visualization)

### 2.1 String Meters (Text Display)

```ini
[MeterText]
Meter=String
MeasureName=MeasureCPU
X=10
Y=40
W=190
H=14
FontFace=Treb uchet MS
FontSize=10
FontColor=255,255,255,205
StringStyle=Bold        ; Normal, Bold, Italic, BoldItalic
StringEffect=Shadow     ; None, Shadow, Border
FontEffectColor=0,0,0,50
StringAlign=Left        ; Left, Center, Right, LeftTop, CenterCenter, RightBottom, etc.
StringCase=Upper        ; None, Upper, Lower, Proper
AntiAlias=1
ClipString=1
Text=%1%               ; %1 = first measure, %2 = second, etc.
Percentual=1           ; Convert bytes to percentage
AutoScale=1            ; Auto-scale bytes (1, 1k, 2k)
NumOfDecimals=1
```

**Advanced String Features:**
- Multiple measure references (%1, %2, %3...)
- Inline formatting (color, size, face changes mid-string)
- Dynamic color based on value
- Tooltip support
- Mouse actions (click, scroll, hover)
- String padding and alignment

### 2.2 Bar Meters (Progress Bars)

```ini
[MeterBar]
Meter=Bar
MeasureName=MeasureCPU
X=10
Y=52
W=190
H=1
BarColor=235,170,0,255
BarOrientation=HORIZONTAL    ; HORIZONTAL or VERTICAL
SolidColor=255,255,255,15    ; Background color
Flip=0                       ; Flip direction
```

**Patterns:**
- Horizontal/vertical orientation
- Solid or gradient fills
- Background colors
- Flip for right-to-left or bottom-to-top
- Used extensively for CPU/RAM/Network

### 2.3 Line Meters (Graphs)

```ini
[MeterLine]
Meter=Line
MeasureName=MeasureCPU
MeasureName2=MeasureRAM
X=10
Y=60
W=190
H=50
LineCount=2
LineColor=255,0,0,255
LineColor2=0,255,0,255
LineWidth=2
HorizontalLines=1
HorizontalLineColor=255,255,255,50
Antialias=1
AutoScale=1
GraphStart=Left
GraphOrientation=Vertical
```

**Patterns:**
- Multiple overlaid lines
- Horizontal grid lines
- Auto-scaling Y-axis
- Scrolling graph (left-to-right)
- Used for CPU history, network traffic

### 2.4 Histogram Meters

```ini
[MeterHistogram]
Meter=Histogram
MeasureName=MeasureCPU
X=10
Y=60
W=190
H=50
PrimaryColor=255,0,0,255
SecondaryColor=0,255,0,255
Flip=0
AutoScale=1
GraphStart=Right
GraphOrientation=Vertical
```

**Patterns:**
- Similar to Line but filled
- Primary/secondary color for gradients
- Used for bandwidth monitors

### 2.5 Roundline Meters (Circular Gauges)

```ini
[MeterRoundline]
Meter=Roundline
MeasureName=MeasureCPU
X=15
Y=15
W=160
H=160
StartAngle=1.5707963    ; Radians: 0=right, π/2=bottom, π=left, 3π/2=top
RotationAngle=4.7123889 ; Arc length in radians
LineStart=65
LineLength=67
LineColor=0,0,0,255
Solid=1                 ; 0=outline, 1=filled
AntiAlias=1
```

**Patterns:**
- Used extensively in Dashboard suite
- Circular CPU/RAM/GPU meters
- Clock faces
- Volume controls
- Speedometer-style gauges
- Start angle + rotation defines arc
- Multiple concentric rings for multi-value displays

### 2.6 Rotator Meters (Needle Gauges)

```ini
[MeterRotator]
Meter=Rotator
MeasureName=MeasureCPU
ImageName=needle.png
X=100
Y=100
W=10
H=50
OffsetX=5
OffsetY=45
StartAngle=0
RotationAngle=6.2831853    ; Full circle (2π)
```

**Patterns:**
- Analog-style needles
- Clock hands
- Speedometer needles
- Rotation based on measure value

### 2.7 Image Meters

```ini
[MeterImage]
Meter=Image
ImageName=#@#Images\background.png
X=0
Y=0
W=200
H=150
PreserveAspectRatio=0
Greyscale=0
ImageTint=255,255,255,255
ImageAlpha=255
ImageFlip=None          ; None, Horizontal, Vertical, Both
ImageRotate=0           ; Degrees
MaskImageName=#@#Images\mask.png
```

**Dynamic Images:**
```ini
[MeterWeatherIcon]
Meter=Image
MeasureName=MeasureWeatherIcon
ImageName=#@#Weather\%1.png    ; %1 replaced with measure value
PreserveAspectRatio=1
```

**Patterns:**
- Background images
- Icons (weather, status, player controls)
- Album artwork
- Dynamic image selection based on measure
- Image effects (tint, alpha, greyscale, rotation)
- Masking for custom shapes

### 2.8 Shape Meters (Vector Graphics)

```ini
[MeterShape]
Meter=Shape
X=0
Y=0
Shape=Rectangle 0,0,200,100,12 | Fill Color 0,0,0,128 | StrokeWidth 2 | Stroke Color 255,255,255,255
Shape2=Ellipse 100,50,30,30 | Fill Color 255,0,0,255
Shape3=Line 0,0,200,100 | StrokeWidth 3 | Stroke Color 0,255,0,255
Shape4=Path MyPath | Fill Color 0,0,255,255
MyPath=10,10 | LineTo 50,50 | LineTo 10,50 | ClosePath 1
DynamicVariables=1
```

**Patterns:**
- Modern vector-based drawing
- Rectangles, ellipses, lines, paths, arcs
- Gradients (linear, radial)
- Transforms (rotate, scale, skew)
- Dynamic shape generation via variables
- GPU-accelerated with D2D (UseD2D=1)

---

## 3. PLUGIN ECOSYSTEM

### 3.1 Native Plugins Discovered

**File:** `C:\Program Files\Rainmeter\Plugins\`

1. **ActionTimer.dll** - Timed action sequences
2. **AdvancedCPU.dll** - Extended CPU metrics
3. **AudioLevel.dll** - Audio visualization (FFT, bands, waveform)
4. **CoreTemp.dll** - CPU temperature monitoring
5. **FileView.dll** - File system browsing
6. **FolderInfo.dll** - Directory statistics
7. **InputText.dll** - Text input dialogs
8. **iTunesPlugin.dll** - iTunes integration
9. **PerfMon.dll** - Windows Performance Monitor
10. **PingPlugin.dll** - Network ping/latency
11. **PowerPlugin.dll** - Battery/power state
12. **QuotePlugin.dll** - Random quotes/text
13. **ResMon.dll** - Resource monitoring
14. **RunCommand.dll** - Execute shell commands, capture output
15. **SpeedFanPlugin.dll** - SpeedFan integration
16. **UsageMonitor.dll** - Windows performance counters
17. **Win7AudioPlugin.dll** - Windows audio control
18. **WindowMessagePlugin.dll** - Inter-process communication

### 3.2 Third-Party Plugins Found in Skins

- **HWiNFO.dll** - Hardware sensors (most comprehensive)
- **NowPlaying.dll** - Universal media player control
- **WebParser.dll** - HTTP requests, regex parsing
- **SysInfo.dll** - System information
- **FrostedGlass.dll** - Windows 10 blur effects

### 3.3 RunCommand Plugin (Execute & Parse)

```ini
[MeasureRun]
Measure=Plugin
Plugin=RunCommand
Program=powershell.exe
Parameter=-Command "Get-Process | Where-Object {$_.CPU -gt 10} | Select-Object -First 5 Name,CPU"
OutputType=ANSI
State=Hide
FinishAction=[!UpdateMeter MeterOutput]
```

**Use Cases:**
- Execute PowerShell/CMD scripts
- Parse command output
- System automation
- Custom data collection
- File operations

---

## 4. VISUALIZATION PATTERNS

### 4.1 Layout Systems

#### Grid-Based Layout (Dashboard Suite)
```ini
[Variables]
GridSize=20
ColumnWidth=200
RowHeight=150
Spacing=10

[MeterWidget1]
X=(#GridSize#*0)
Y=(#GridSize#*0)

[MeterWidget2]
X=(#GridSize#*11)    ; 11 columns over
Y=0r                  ; Same Y as previous (relative)
```

#### Relative Positioning
```ini
[MeterTitle]
X=100
Y=12

[MeterValue]
X=200
Y=0r          ; r = relative to previous meter's Y
```

#### Anchoring
```ini
[MeterBase]
X=100
Y=100

[MeterAnchored]
X=10r         ; 10 pixels right of MeterBase
Y=0r          ; Same Y as MeterBase
```

### 4.2 Color Schemes & Themes

#### Variable-Based Theming
```ini
[Variables]
; Dark theme
ColorBG=0,0,0,200
ColorText=255,255,255,205
ColorAccent=235,170,0,255
ColorBar=235,170,0,255
ColorShadow=0,0,0,50

; Font settings
FontName=Trebuchet MS
FontSize=10
```

#### Dynamic Color Based on Value
```ini
[MeasureColorCalc]
Measure=Calc
Formula=(MeasureCPU > 80) ? -1 : ((MeasureCPU > 50) ? -2 : -3)
Substitute="-1":"255,0,0,255","-2":"255,255,0,255","-3":"0,255,0,255"

[MeterText]
Meter=String
FontColor=[MeasureColorCalc]
DynamicVariables=1
```

### 4.3 Animation Patterns

#### Smooth Transitions (ActionTimer)
```ini
[MeasureSlide]
Measure=Plugin
Plugin=ActionTimer
ActionList1=Repeat MoveRight, 30, 100
MoveRight=[!SetOption MeterWidget X "(#X#+5)"][!UpdateMeter *][!Redraw]
```

#### Fade Effects
```ini
[MeasureFadeIn]
Measure=Calc
Formula=(MeasureFadeIn < 255) ? (MeasureFadeIn + 5) : 255
DynamicVariables=1

[MeterFading]
ImageAlpha=[MeasureFadeIn]
DynamicVariables=1
```

### 4.4 Conditional Display

```ini
[MeterWarning]
Meter=String
Text=HIGH CPU USAGE!
Hidden=(MeasureCPU < 80) ? 1 : 0
DynamicVariables=1
```

### 4.5 Mouse Interactions

```ini
[MeterInteractive]
Meter=String
LeftMouseUpAction=[!ToggleMeter MeterDetails]
RightMouseUpAction=[!SkinCustomMenu]
MouseScrollUpAction=[!SetVariable Volume "(#Volume#+5)"][!UpdateMeter *][!Redraw]
MouseScrollDownAction=[!SetVariable Volume "(#Volume#-5)"][!UpdateMeter *][!Redraw]
MouseOverAction=[!SetOption MeterInteractive FontColor "255,255,0,255"][!UpdateMeter *][!Redraw]
MouseLeaveAction=[!SetOption MeterInteractive FontColor "#ColorText#"][!UpdateMeter *][!Redraw]
ToolTipText="Click for details"
```

---

## 5. WIDGET TAXONOMY

### 5.1 System Monitoring Widgets

**CPU Widgets:**
- Single core usage
- Multi-core visualization (4, 8, 16+ cores)
- CPU temperature
- Clock speed
- Process list/top consumers
- Historical graphs

**GPU Widgets:**
- GPU load (compute + graphics)
- GPU temperature
- Clock speeds (core, memory, shader)
- VRAM usage
- Fan speed
- Power consumption
- Multi-GPU support

**Memory Widgets:**
- Physical RAM usage
- Swap/page file
- Per-process memory
- Memory speed/timings

**Disk Widgets:**
- Free/used space per drive
- Disk I/O (read/write MB/s)
- Disk activity percentage
- SMART data
- Multi-drive grids

**Network Widgets:**
- Upload/download speed
- Total bandwidth
- IP address (LAN/WAN)
- Active connections
- Ping/latency
- Network interface selection

### 5.2 Media & Entertainment Widgets

**Music Players:**
- Album artwork
- Track info (title, artist, album, year)
- Playback controls
- Progress bar
- Volume control
- Playlist management
- Shuffle/repeat indicators
- Supported: Winamp, iTunes, Spotify, foobar2000, WMP, MusicBee, MediaMonkey

**Audio Visualizers:**
- Spectrum analyzers (bar, line, circular)
- Waveform displays
- VU meters
- Peak meters
- Frequency band customization
- Color gradients
- Pulsing effects

### 5.3 Information & Productivity Widgets

**Clocks & Calendars:**
- Analog clocks (traditional, modern, minimal)
- Digital clocks (12/24 hour)
- World clocks (multiple time zones)
- Calendars (month view, agenda)
- Date displays
- Countdown timers

**Weather Widgets:**
- Current conditions
- Temperature (current, feels like, min/max)
- Weather icons
- 5-10 day forecast
- Hourly forecast
- Radar maps (animated)
- Alerts
- Multiple locations

**RSS/News Feeds:**
- Headline tickers
- Article summaries
- Auto-refresh
- Category filtering
- Multiple feed support

**System Info Widgets:**
- Computer name
- OS version
- Uptime
- User name
- Boot time
- Last update

### 5.4 Utility & Control Widgets

**Volume Controls:**
- Master volume
- App-specific volume
- Mute toggle
- Visual feedback (bars, rings, sliders)

**Launchers:**
- Application shortcuts
- Folder shortcuts
- Website links
- Icon grids/docks
- Context menus
- Game launchers with artwork

**Process Managers:**
- Top N CPU/memory consumers
- Kill process action
- Process count
- Thread count

**Clipboard Managers:**
- Clipboard history
- Quick paste actions
- Text formatting

**Notes & Reminders:**
- Sticky notes
- Todo lists
- Countdown reminders
- Event calendars

**Calculators:**
- Basic/scientific modes
- Unit converters
- Currency converters

### 5.5 Specialized & Advanced Widgets

**Command Terminals:**
- PowerShell/CMD execution
- Output display
- Command history
- Customizable commands

**File Browsers:**
- Directory listing
- File operations
- Quick access
- Search

**Lock Screens:**
- Password protection
- Slideshow backgrounds
- Clock/date display

**Interactive Dashboards:**
- Multi-widget layouts
- Tab systems
- Hide/show panels
- Settings panels

**Themed Suites:**
- Fallout Terminal
- Sci-fi HUDs
- Minimal/flat design
- Skeuomorphic designs
- Game-themed (Cyberpunk, etc.)

---

## 6. ADVANCED TECHNIQUES

### 6.1 Performance Optimization

```ini
[Rainmeter]
Update=1000              ; Base update rate
UpdateDivider=60         ; This meter updates every 60 seconds
DynamicWindowSize=1
UseD2D=1                 ; Hardware acceleration
```

### 6.2 Multi-Skin Communication

```ini
; Skin A broadcasts
[MeasureBroadcast]
Measure=Calc
Formula=MeasureCPU
OnChangeAction=[!SetVariable CPUValue "[MeasureBroadcast]" "#ROOTCONFIG#\SkinB"][!UpdateMeasure * "#ROOTCONFIG#\SkinB"]

; Skin B receives
[Variables]
CPUValue=0
```

### 6.3 Settings Management

```ini
[Variables]
@include=#SKINSPATH#Config\Settings.inc

; Settings.inc:
ColorScheme=Dark
ShowAdvanced=1
UpdateInterval=1000
```

### 6.4 Localization

```ini
[Variables]
@include=#@#Languages\#Language#.inc

; English.inc:
LabelCPU=CPU Usage
LabelRAM=Memory

; Spanish.inc:
LabelCPU=Uso de CPU
LabelRAM=Memoria
```

---

## 7. DATA COLLECTION PATTERNS

### 7.1 Web API Integration

**Pattern: WebParser + JSON**
```ini
[MeasureAPI]
Measure=Plugin
Plugin=WebParser
Url=https://api.example.com/data
RegExp="(?siU).*\"value\":\"(.*)\".*"
UpdateRate=600

[MeasureValue]
Measure=Plugin
Plugin=WebParser
Url=[MeasureAPI]
StringIndex=1
```

### 7.2 Registry Reading

```ini
[MeasureRegistry]
Measure=Registry
RegHKey=HKEY_CURRENT_USER
RegKey=Software\MyApp
RegValue=Setting1
```

### 7.3 File Parsing

```ini
[MeasureFile]
Measure=Plugin
Plugin=FileView
Path=C:\Logs\
Type=FileCount
Pattern=*.log
Recursive=1
```

### 7.4 Performance Counter Access

```ini
[MeasurePerfMon]
Measure=Plugin
Plugin=UsageMonitor
Category=Processor
Counter=% Processor Time
Instance=_Total
```

---

## 8. RECOMMENDED C++ WIDGET ARCHITECTURE

Based on all patterns analyzed, here's the recommended structure:

### 8.1 Widget Type Hierarchy

```
BaseWidget (abstract)
├── SystemMonitorWidget (abstract)
│   ├── CPUWidget
│   ├── GPUWidget
│   ├── MemoryWidget
│   ├── DiskWidget
│   └── NetworkWidget
├── MediaWidget (abstract)
│   ├── MusicPlayerWidget
│   └── AudioVisualizerWidget
├── InformationWidget (abstract)
│   ├── ClockWidget
│   ├── WeatherWidget
│   └── CalendarWidget
├── UtilityWidget (abstract)
│   ├── LauncherWidget
│   ├── VolumeControlWidget
│   └── ProcessManagerWidget
└── CustomWidget (Lua/script-driven)
```

### 8.2 Required Measure Types

```cpp
enum class MeasureType {
    CPU,                // Measure=CPU
    Memory,             // Measure=PhysicalMemory
    Disk,               // Measure=FreeDiskSpace
    Network,            // Measure=NetIn/NetOut
    Time,               // Measure=Time
    Plugin,             // Measure=Plugin
    Registry,           // Measure=Registry
    Calc,               // Measure=Calc
    Script,             // Measure=Script (Lua)
    WebParser,          // HTTP requests
    Counter             // Performance counters
};
```

### 8.3 Required Meter Types

```cpp
enum class MeterType {
    String,             // Text display
    Bar,                // Progress bar
    Line,               // Line graph
    Histogram,          // Histogram graph
    Roundline,          // Circular gauge
    Rotator,            // Needle gauge
    Image,              // Static/dynamic images
    Shape,              // Vector shapes (D2D)
    Button              // Interactive button
};
```

### 8.4 Plugin Interface

```cpp
class IWidgetPlugin {
public:
    virtual bool Initialize() = 0;
    virtual void Update() = 0;
    virtual double GetValue() = 0;
    virtual std::wstring GetStringValue() = 0;
    virtual void ExecuteCommand(const std::wstring& command) = 0;
    virtual ~IWidgetPlugin() = default;
};
```

---

## 9. KEY FINDINGS & RECOMMENDATIONS

### 9.1 Essential Features for RainmeterManager

**Must Have:**
1. Hardware monitoring (CPU, GPU, RAM, Disk, Network) via WMI/PDH
2. Media player control (universal plugin architecture)
3. Audio visualization (FFT/frequency band analysis)
4. Web data fetching (HTTP client + regex/JSON parsing)
5. Vector shape rendering (Direct2D integration)
6. Lua scripting engine integration
7. Dynamic configuration (variables, includes, themes)
8. Mouse interaction handling
9. Animation system (transitions, fades)
10. Plugin architecture (DLL loading, C API)

**Should Have:**
1. Hardware sensor integration (HWiNFO compatibility)
2. Multiple layout engines (grid, flow, absolute)
3. Skin theming system
4. Multi-skin communication
5. Settings persistence (INI/JSON)
6. Localization support
7. Performance optimization (update dividers, caching)

**Nice to Have:**
1. Visual skin editor
2. Online skin repository
3. Automatic updates
4. Backup/restore
5. Performance profiling tools

### 9.2 Performance Considerations

- Use hardware acceleration (Direct2D) for all rendering
- Implement update dividers (some widgets update every 60s, not 1s)
- Cache expensive operations (regex, web requests)
- Lazy initialization of widgets
- Thread pool for background data collection
- Efficient string handling (avoid constant conversions)

### 9.3 Security Considerations

- Sandbox web requests (whitelist domains)
- Validate all regex patterns
- Limit script execution time
- Validate file paths (prevent directory traversal)
- Sign all plugins
- Implement permission system for sensitive operations

---

## 10. NEXT STEPS

1. ✅ Catalog complete
2. ⏳ Create enhanced C++ widget template classes
3. ⏳ Implement measure/meter base classes
4. ⏳ Design plugin API
5. ⏳ Create sample widget implementations
6. ⏳ Develop layout engine
7. ⏳ Integrate with existing RainmeterManager framework

---

**Document prepared by:** Warp AI Agent  
**For project:** RainmeterManager Enhanced Widget System  
**Total analysis time:** 30 minutes across 962 files
