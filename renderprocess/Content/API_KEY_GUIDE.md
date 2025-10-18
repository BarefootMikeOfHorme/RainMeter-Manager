# API Key Registration Guide

Complete guide to obtain API keys for all integrated services in RainmeterManager content loaders.

## 🔑 Required API Keys

### 1. NASA API ⭐ HIGHLY RECOMMENDED
**Used for:** 7 integrated services
- Astronomy Picture of the Day (APOD)
- Mars Rover Photos (Perseverance & Curiosity)  
- Earth from Space (EPIC satellite)
- NASA Image Library search
- Random astronomy pictures

**Sign up:** https://api.nasa.gov/  
**Cost:** FREE (1000 requests/hour)  
**Approval:** INSTANT  
**Environment Variable:** `NASA_API_KEY`

**Steps:**
1. Visit https://api.nasa.gov/
2. Click "Generate API Key"
3. Fill in: First Name, Last Name, Email
4. Check email for API key (instant)
5. Copy key and save

**Example Response:**
```
Your API key for john.doe@example.com is:
ABC123def456GHI789jkl012MNO345pqr678STU
```

---

### 2. OpenWeatherMap ⭐ HIGHLY RECOMMENDED
**Used for:** Current weather conditions, forecasts  
**Sign up:** https://home.openweathermap.org/users/sign_up  
**Cost:** FREE tier (60 calls/min, 1M calls/month)  
**Approval:** ~10 minutes - 2 hours  
**Environment Variable:** `OPENWEATHER_API_KEY`

**Steps:**
1. Visit https://home.openweathermap.org/users/sign_up
2. Create account with email verification
3. Go to https://home.openweathermap.org/api_keys
4. Copy "Default" API key or create new one
5. **IMPORTANT:** Wait 10 minutes - 2 hours for activation

**Test Your Key:**
```powershell
curl "https://api.openweathermap.org/data/2.5/weather?q=London&appid=YOUR_KEY&units=metric"
```

---

### 3. WeatherAPI.com
**Used for:** Real-time weather with forecasts  
**Sign up:** https://www.weatherapi.com/signup.aspx  
**Cost:** FREE tier (1M calls/month)  
**Approval:** INSTANT  
**Environment Variable:** `WEATHER_API_KEY`

**Steps:**
1. Visit https://www.weatherapi.com/signup.aspx
2. Fill registration form
3. Email confirmation link (instant)
4. Login and copy API key from dashboard
5. Key works immediately

---

### 4. World Air Quality Index (WAQI) ⭐ RECOMMENDED
**Used for:** Real-time air quality data worldwide  
**Sign up:** https://aqicn.org/data-platform/token/  
**Cost:** FREE (reasonable use)  
**Approval:** INSTANT  
**Environment Variable:** `WAQI_TOKEN`

**Steps:**
1. Visit https://aqicn.org/data-platform/token/
2. Enter your email address
3. Click "Submit"
4. Check email for token (arrives in seconds)
5. Token format: `demo` or alphanumeric string

**Test Your Token:**
```powershell
curl "https://api.waqi.info/feed/washington/?token=YOUR_TOKEN"
```

---

### 5. Rijksmuseum API (Optional)
**Used for:** Dutch art collection access  
**Sign up:** https://data.rijksmuseum.nl/object-metadata/api/  
**Cost:** FREE (10,000 requests/day)  
**Approval:** Email confirmation required  
**Environment Variable:** `RIJKSMUSEUM_API_KEY`

**Steps:**
1. Visit https://data.rijksmuseum.nl/object-metadata/api/
2. Click "Request API key"
3. Fill in application form (name, email, purpose)
4. Check email for API key
5. Activation may take 24-48 hours

---

## 📋 Services That DON'T Need Keys (Already Working)

These services are integrated and work without API keys:

### Space & Science
- ✅ SpaceX Launch Data (spacexdata.com)
- ✅ ISS Location Tracking (open-notify.org)
- ✅ USGS Earthquake Data (earthquake.usgs.gov)
- ✅ NASA Image Library (basic search)

### Weather
- ✅ NOAA Weather Radar (weather.gov)
- ✅ NOAA Weather Forecasts (weather.gov API)
- ✅ Open-Meteo Forecasts (open-meteo.com)
- ✅ NOAA Tides & Currents

### News & Social
- ✅ Reddit (public JSON endpoints)
- ✅ Hacker News (firebase API)
- ✅ Wikipedia Featured Articles
- ✅ Wikimedia Commons Picture of Day

### Finance
- ✅ Bitcoin Price Index (coindesk.com)
- ✅ Crypto Fear & Greed Index (alternative.me)
- ✅ Currency Exchange Rates (exchangerate-api.com)

### Art & Culture
- ✅ Metropolitan Museum of Art (metmuseum.org)
- ✅ Art Institute of Chicago (artic.edu)

### Maps & Geography
- ✅ OpenStreetMap Tiles
- ✅ Windy Weather Maps

### Education
- ✅ arXiv Research Papers
- ✅ Numbers API (math facts)
- ✅ ZenQuotes Daily Quotes

---

## ⚙️ How to Configure API Keys

### Method 1: Environment Variables (Recommended for Development)

**Windows PowerShell (Session-only):**
```powershell
$env:NASA_API_KEY = "your_key_here"
$env:OPENWEATHER_API_KEY = "your_key_here"
$env:WEATHER_API_KEY = "your_key_here"
$env:WAQI_TOKEN = "your_token_here"
$env:RIJKSMUSEUM_API_KEY = "your_key_here"
```

**Windows PowerShell (Permanent - User Level):**
```powershell
[System.Environment]::SetEnvironmentVariable('NASA_API_KEY', 'your_key_here', 'User')
[System.Environment]::SetEnvironmentVariable('OPENWEATHER_API_KEY', 'your_key_here', 'User')
[System.Environment]::SetEnvironmentVariable('WEATHER_API_KEY', 'your_key_here', 'User')
[System.Environment]::SetEnvironmentVariable('WAQI_TOKEN', 'your_token_here', 'User')
```

**Restart your terminal** after setting permanent variables.

### Method 2: appsettings.json (For Production)

Create or edit `D:\RainmeterManager\renderprocess\appsettings.json`:

```json
{
  "ContentLoaders": {
    "WebContentLoader": {
      "ApiKeys": {
        "NASA_API_KEY": "YOUR_NASA_KEY_HERE",
        "OPENWEATHER_API_KEY": "YOUR_OPENWEATHER_KEY_HERE",
        "WEATHER_API_KEY": "YOUR_WEATHERAPI_KEY_HERE",
        "WAQI_TOKEN": "YOUR_WAQI_TOKEN_HERE",
        "RIJKSMUSEUM_API_KEY": "YOUR_RIJKS_KEY_HERE"
      }
    }
  }
}
```

Then modify `WebContentLoader.cs` to read from configuration instead of environment variables.

### Method 3: Use Configuration Script

```powershell
# Generate template
.\configure_loaders.ps1 -GenerateTemplate

# Edit appsettings.json with your keys

# Validate
.\configure_loaders.ps1 -ValidateOnly
```

---

## 🎯 Quick Start: Get All Free Keys in 10 Minutes

### Priority Order:

**1. NASA API (2 minutes)**
- Instant approval, most services
- https://api.nasa.gov/

**2. WAQI Air Quality (1 minute)**
- Instant email delivery
- https://aqicn.org/data-platform/token/

**3. WeatherAPI.com (2 minutes)**
- Quick signup, instant key
- https://www.weatherapi.com/signup.aspx

**4. OpenWeatherMap (2 minutes signup + 30 min wait)**
- Standard weather provider
- https://home.openweathermap.org/users/sign_up
- ⚠️ Activation delay: wait 10 min - 2 hours

**5. Rijksmuseum (optional, 3 minutes)**
- For art enthusiasts
- https://data.rijksmuseum.nl/object-metadata/api/

---

## 🧪 Testing Your Keys

### Test NASA API:
```powershell
curl "https://api.nasa.gov/planetary/apod?api_key=YOUR_NASA_KEY"
```
**Expected:** JSON with today's astronomy picture

### Test OpenWeatherMap:
```powershell
curl "https://api.openweathermap.org/data/2.5/weather?q=London&appid=YOUR_KEY&units=metric"
```
**Expected:** JSON with London weather data

### Test WeatherAPI:
```powershell
curl "https://api.weatherapi.com/v1/current.json?key=YOUR_KEY&q=London"
```
**Expected:** JSON with current weather

### Test WAQI:
```powershell
curl "https://api.waqi.info/feed/washington/?token=YOUR_TOKEN"
```
**Expected:** JSON with air quality index

---

## 🔒 Security Best Practices

### ⚠️ DO NOT:
- ❌ Commit API keys to version control (add appsettings.json to .gitignore)
- ❌ Share keys publicly or in screenshots
- ❌ Use production keys in development logs

### ✅ DO:
- ✅ Use environment variables for local development
- ✅ Rotate keys periodically (every 6-12 months)
- ✅ Use separate keys for dev/staging/production
- ✅ Monitor usage on provider dashboards
- ✅ Set up usage alerts if available

---

## 📊 API Key Usage Summary

| Service | Free Tier Limit | Used For | Priority |
|---------|----------------|----------|----------|
| NASA | 1000 req/hour | 7 space sources | ⭐⭐⭐ Essential |
| OpenWeatherMap | 60 req/min | Weather data | ⭐⭐⭐ Essential |
| WeatherAPI | 1M req/month | Weather forecasts | ⭐⭐ Recommended |
| WAQI | Reasonable use | Air quality | ⭐⭐ Recommended |
| Rijksmuseum | 10K req/day | Art collection | ⭐ Optional |

---

## 🆘 Troubleshooting

### "Invalid API key" errors:

**OpenWeatherMap:**
- Wait 10 minutes - 2 hours after signup
- Check key at https://home.openweathermap.org/api_keys
- Verify email confirmation

**NASA:**
- Keys work instantly
- If not working, regenerate at https://api.nasa.gov/

**WAQI:**
- Check spam folder for email
- Token should arrive within 60 seconds

### "Rate limit exceeded":

- NASA: Wait until next hour (resets hourly)
- OpenWeatherMap: Wait 1 minute (60 calls/min limit)
- Implement caching in WebContentLoader (already configured)

### Keys not being used:

Check environment variables are set:
```powershell
Get-ChildItem Env: | Where-Object { $_.Name -like '*API*' -or $_.Name -like '*TOKEN*' }
```

---

## 📝 Next Steps

After obtaining keys:

1. ✅ Set environment variables or update appsettings.json
2. ✅ Run `.\verify_loaders.ps1` to test integration
3. ✅ Deploy loaders with `.\deploy_loaders.ps1 -Force`
4. ✅ Test a few API-enabled sources in your render process
5. ✅ Monitor API usage on provider dashboards

---

**Last Updated:** 2025-10-19  
**Status:** Production Ready
