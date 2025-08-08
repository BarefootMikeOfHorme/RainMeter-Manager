# Rainmeter Skin Collection

This collection contains popular Rainmeter skins that have been featured in desktop customization communities and articles. Each skin is designed to enhance your Windows desktop experience with beautiful widgets and system monitoring tools.

## Available Skins

### 1. Mond
**Author:** HiTBiT  
**Type:** Minimalist Clock & System Info  
An elegant and clean skin featuring time, date, weather, and basic system information in a minimalist design. Perfect for users who prefer a subtle desktop enhancement.

**Features:**
- Large, readable time display
- Current date
- Weather information (requires API key)
- CPU and RAM usage
- Clean, transparent background

### 2. Cleartext
**Author:** Redsaph  
**Type:** Music Player Interface  
A music visualizer and now-playing display that works with Spotify, iTunes, WMP, and many other media players.

**Features:**
- Album artwork display
- Song title, artist, and album information
- Playback controls (play/pause, previous, next)
- Progress bar
- Transparent, overlay-friendly design

### 3. SimpleMedia
**Author:** lilshizzy  
**Type:** System Monitor  
A straightforward system monitoring skin that displays CPU, RAM, and disk usage with clean progress bars.

**Features:**
- Real-time CPU usage monitoring
- RAM usage display
- Disk space monitoring (C: drive)
- System uptime counter
- Compact, unobtrusive design

### 4. Translucent
**Author:** Fediafedia  
**Type:** Advanced System Monitor  
Beautiful translucent system monitoring skin with elegant design and detailed information.

**Features:**
- Multi-core CPU monitoring
- Detailed memory usage (RAM + Swap)
- Disk space tracking
- Translucent background effect
- Color-coded progress bars

### 5. Honeycomb
**Author:** APIIUM  
**Type:** Application Launcher  
Honeycomb-shaped application launcher with customizable icons for quick access to your favorite programs.

**Features:**
- Quick launch for popular applications
- Hover effects
- Customizable icon arrangement
- Support for various applications (Chrome, Spotify, Steam, Discord, etc.)
- *Note: Icon images need to be added separately*

### 6. CircuitousTwo Weather
**Author:** flyinghyrax  
**Type:** Weather Display  
Clean and elegant weather display with current conditions and basic forecast information.

**Features:**
- Current temperature and conditions
- Weather description
- "Feels like" temperature
- High/low temperatures
- Location display
- *Requires OpenWeatherMap API key*

### 7. Elegance2 Clock
**Author:** lilshizzy  
**Type:** Analog/Digital Clock  
Elegant combination of analog and digital clock displays with smooth animations.

**Features:**
- Traditional analog clock face
- Moving hour, minute, and second hands
- Digital time display
- Current date display
- Smooth, anti-aliased graphics

## Installation Instructions

1. **Install Rainmeter** if you haven't already from [rainmeter.net](https://www.rainmeter.net/)

2. **Copy Skins:** Copy the skin folders to your Rainmeter Skins directory:
   - Default location: `Documents\Rainmeter\Skins\`
   - Or use the skin installer (.rmskin files) if available

3. **Refresh Rainmeter:** Right-click the Rainmeter icon in your system tray and select "Refresh all"

4. **Load Skins:** Right-click the Rainmeter icon and navigate to the skins you want to load

## Configuration

### Weather Skins (Mond, CircuitousTwo)
To use weather features, you'll need to:
1. Sign up for a free API key at [OpenWeatherMap](https://openweathermap.org/api)
2. Edit the Variables.inc file (for Mond) or the skin .ini file directly
3. Replace `YourOpenWeatherMapAPIKey` with your actual API key
4. Set your location (city name or coordinates)

### Music Player Skins (Cleartext)
- Most music players are supported automatically
- For Spotify, make sure it's running
- Some players might require additional plugins

### Application Launcher (Honeycomb)
- Add your own 80x80 pixel PNG icons to the `@Resources\Images\` folder
- Edit the skin file to update application paths if they differ from defaults
- Icons can be downloaded from icon sites like Flaticon, Icons8, or IconArchive

## Customization

All skins can be customized by editing their .ini files:
- **Colors:** Modify RGB values in the Variables section
- **Positions:** Change X and Y coordinates of meters
- **Fonts:** Update FontFace properties
- **Sizes:** Adjust FontSize and meter dimensions

## Troubleshooting

**Skins not loading:**
- Check that files are in the correct Skins directory
- Refresh Rainmeter after adding new skins
- Ensure .ini files have proper syntax

**Weather not working:**
- Verify your API key is correct and active
- Check your internet connection
- Ensure location is spelled correctly

**Icons missing in Honeycomb:**
- Add actual PNG image files to replace .txt placeholders
- Ensure images are named correctly (Chrome.png, Spotify.png, etc.)
- Icons should be 80x80 pixels for best results

## Credits

This collection is inspired by the article "7 Rainmeter skins I've used to transform my desktop" and represents some of the most popular skins in the Rainmeter community. All skins are created by their respective authors and distributed under Creative Commons licenses.

## License

Each skin maintains its original license as specified in the metadata. Most are under Creative Commons Attribution-Non-Commercial-Share Alike 3.0 or similar open-source licenses.

---

*Last updated: July 2025*
