# RainmeterManager Sandboxing Philosophy
## "Safe Creativity" - Security Through Education and Empowerment

### Core Principles

**We believe in:**
- **Educate, Don't Restrict** - Help users understand risks, then let them decide
- **Trust User Signatures** - If you sign it, we trust it
- **Community Wisdom** - Share knowledge about safe plugins
- **Transparency** - Always explain what's happening and why

**We don't:**
- Silently block user content
- Make security an obstacle to creativity
- Hide technical details from power users
- Force one-size-fits-all policies

---

## Smart DLL Validation

### How It Works

When a plugin/DLL is loaded, we:

1. **Check Signature**
   - Microsoft-signed → ✅ Automatic trust
   - User's certificate → ✅ Automatic trust (you signed it!)
   - Known publisher → ✅ Automatic trust
   - Unsigned → ⚠️ Ask user

2. **Check Community Database**
   - Popular plugin with good hash → ✅ Trusted
   - Modified version → ⚠️ Show differences, let user decide

3. **Check User's Previous Decisions**
   - You approved it before → ✅ Remember your choice
   - You blocked it before → 🚫 Respect your decision

4. **Present Risk Assessment**
   ```
   Plugin: AwesomeVisualizer.dll
   Status: Unsigned
   Hash: abc123...
   Risk Level: Medium
   
   Why Medium?
   - No digital signature found
   - Not in community whitelist
   - File size: 2.5MB (reasonable)
   - First time loading
   
   Your Options:
   [ ] Trust this version only
   [ ] Trust all versions (by name)
   [ ] Block this DLL
   [ ] Sign it yourself (we can help!)
   
   [Learn How to Sign DLLs] [Community Reviews]
   ```

### Helping Users Sign Their Own DLLs

We provide tools and guidance:

```powershell
# RainmeterManager includes helpers:
./SignMyPlugin.ps1 -DLL "MyPlugin.dll" -CreateCert

# Or use Windows built-in tools:
New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=YourName"
signtool sign /a /t http://timestamp.digicert.com "MyPlugin.dll"
```

**Why sign your DLLs?**
- You trust your own work
- Others can verify it came from you
- Updates are automatically trusted
- Professional development practice

---

## Network Isolation

### Default: Localhost Only

Render processes default to **LocalhostOnly** mode:
- ✅ IPC with main process
- ✅ Loopback connections (127.0.0.1)
- 🚫 Internet access
- 🚫 LAN access

### When Network Is Needed

If a plugin needs internet:

```
PluginName wants network access

Purpose: Download weather data
Host: api.weather.com

Risk: Low - Read-only API access

Your Options:
[ ] Allow this time only
[ ] Allow always for this plugin
[ ] Allow specific hosts only: [api.weather.com]
[ ] Deny

[Why does this plugin need network?]
```

### Firewall Integration

We generate Windows Firewall rules for you:
```powershell
# Created automatically when you choose "localhost only"
New-NetFirewallRule -DisplayName "RainmeterManager Render Process" `
    -Direction Outbound -Action Block `
    -Program "C:\...\RenderProcess.exe" `
    -RemoteAddress "0.0.0.0-255.255.255.255" `
    -Description "Block internet, allow localhost"
```

---

## Sandboxing Layers

### Layer 1: Resource Limits (Always On)
- **Memory**: 512MB default (configurable)
- **CPU**: 80% max (prevents system freeze)
- **Processes**: 1 (prevents fork bombs)

### Layer 2: Privilege Reduction (Default: On)
Removes dangerous privileges:
- Can't debug other processes
- Can't load drivers
- Can't change system time
- Can't shutdown system
- Can't impersonate other users

### Layer 3: Code Integrity (Optional)
- **Control Flow Guard (CFG)**: ✅ On by default
  - Prevents memory corruption exploits
  - Minimal compatibility issues
  
- **DEP + ASLR**: ✅ On by default
  - Randomizes memory layout
  - Makes exploits harder

- **Dynamic Code Blocking**: ❌ Off by default
  - Would break JavaScript engines, Lua, etc.
  - Enable only if you don't use scripting

### Layer 4: Win32k Lockdown (Optional)
For truly headless rendering (no GUI):
- Blocks legacy Windows UI calls
- Modern DirectX/Vulkan still works
- ❌ Off by default (many skins use GDI+)

### Layer 5: DLL Validation (Smart Mode)
See "Smart DLL Validation" above

---

## Configuration Examples

### Maximum Security (Paranoid Mode)
```cpp
SandboxConfig paranoid;
paranoid.enableMitigations = true;
paranoid.enableCFG = true;
paranoid.enableDynamicCodeProhibit = true;
paranoid.enforceCodeSigning = true;
paranoid.dllPolicy = DLLLoadingPolicy::SignedOnly;
paranoid.networkPolicy = NetworkPolicy::Blocked;
paranoid.reducePrivileges = true;
paranoid.addRestrictedSIDs = true;

// Warning: May break many plugins!
```

### Balanced Security (Recommended)
```cpp
SandboxConfig balanced;
balanced.enableMitigations = true;
balanced.enableCFG = true;
balanced.dllPolicy = DLLLoadingPolicy::ValidatedOnly;
balanced.allowUserSignedDLLs = true;
balanced.networkPolicy = NetworkPolicy::LocalhostOnly;
balanced.reducePrivileges = true;

// Good security, user-friendly
```

### Creative Freedom (Development Mode)
```cpp
SandboxConfig dev;
dev.enableMitigations = false;
dev.dllPolicy = DLLLoadingPolicy::AllowAll;
dev.networkPolicy = NetworkPolicy::FullAccess;
dev.reducePrivileges = false;

// For plugin development and testing
```

---

## Trust Store Management

### User Trust Store
Located: `%APPDATA%\RainmeterManager\TrustStore.json`

```json
{
  "decisions": [
    {
      "dllHash": "abc123...",
      "dllName": "MyAwesomePlugin.dll",
      "trusted": true,
      "reason": "I created this plugin",
      "timestamp": "2025-10-18T10:00:00Z",
      "applyToAllVersions": true
    },
    {
      "dllHash": "def456...",
      "dllName": "ThirdPartyWidget.dll",
      "trusted": true,
      "reason": "Community approved, hash verified",
      "timestamp": "2025-10-18T11:00:00Z",
      "applyToAllVersions": false
    }
  ]
}
```

You can edit this file directly or use the UI.

### Community Database
Shared knowledge about popular plugins:
- Hash whitelists for known-good versions
- Known malicious hashes (blocklist)
- Security notes and recommendations
- Updated via optional sync (never automatic)

### Hash Whitelist
System-level known-safe DLLs:
- Microsoft DLLs
- Popular open-source libraries
- Verified publisher content

---

## For Plugin Developers

### Recommended Practices

1. **Sign Your DLLs**
   - Get a code signing certificate (or create self-signed)
   - Users who install your cert will auto-trust your plugins
   
2. **Declare Permissions**
   - Include manifest describing network/file access needs
   - Users appreciate transparency

3. **Provide Hashes**
   - Publish SHA-256 hashes of releases
   - Users can verify before loading

4. **Community Listing**
   - Submit to community database
   - Builds trust, increases adoption

### Example Plugin Manifest
```json
{
  "name": "WeatherWidget",
  "version": "1.2.0",
  "author": "John Doe",
  "signature": "SHA256:abc123...",
  "permissions": {
    "network": ["api.weather.com"],
    "files": ["read:config.json"],
    "registry": false
  },
  "description": "Displays weather from trusted API"
}
```

---

## Security Without Barriers

### What We Block Automatically
- Known malware hashes
- Blatantly suspicious behavior (e.g., 100MB DLL)
- Privilege escalation attempts

### What We Warn About
- Unsigned DLLs (but allow with approval)
- Network access from render process
- High memory usage
- Unusual file operations

### What We Never Block
- Your own signed DLLs
- Community-approved plugins you approve
- Content you explicitly trust
- Development/testing scenarios

---

## Future Enhancements

### Planned Features
- [ ] Windows Filtering Platform integration (automated firewall rules)
- [ ] Cryptographic plugin checksums
- [ ] Community reputation system
- [ ] Automated security audits
- [ ] Sandboxed preview mode (test plugins safely)
- [ ] Plugin update verification
- [ ] Integrity monitoring (detect tampering)

### Community Contributions Welcome
- Maintain community database
- Report suspicious plugins
- Share safe plugin hashes
- Write security guides
- Improve validation heuristics

---

## Summary

**RainmeterManager's sandboxing is designed to:**
1. Protect you from actual threats
2. Educate you about risks
3. Respect your decisions
4. Encourage signing and verification
5. Foster a secure community
6. Never get in the way of creativity

**Security is not a wall - it's a safety net.**

You're free to climb, experiment, and create. We're just here to catch you if something goes wrong.

---

*"The best security is informed users who control their own systems."*

---

## Quick Reference

| Feature | Default | Recommendation |
|---------|---------|----------------|
| Job Object Limits | ✅ On | Keep on |
| Privilege Reduction | ✅ On | Keep on |
| CFG Mitigation | ✅ On | Keep on |
| DLL Validation | ✅ Smart | Keep smart |
| User-Signed DLLs | ✅ Trusted | Sign your plugins! |
| Network | 🔒 Localhost only | Override per-plugin |
| Dynamic Code | ❌ Off | Enable for strict security |
| Win32k Lockdown | ❌ Off | Enable for headless only |

**Questions?** Check the [Security FAQ](./SECURITY_FAQ.md) or ask the community!
