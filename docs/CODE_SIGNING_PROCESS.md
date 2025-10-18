# Code Signing Process Guide

**Document Version:** 1.0  
**Last Updated:** October 18, 2025  
**Owner:** DevOps Team

---

## Overview

This document outlines the process for acquiring and implementing code signing for RainmeterManager binaries to meet enterprise security requirements and Windows SmartScreen reputation building.

---

## 1. EV Code Signing Certificate Requirements

### 1.1 What is EV Code Signing?

**Extended Validation (EV) Code Signing** provides:
- Immediate Windows SmartScreen reputation
- Higher trust level than standard code signing
- Hardware-backed private key storage (USB token or cloud HSM)
- Organization identity validation

### 1.2 Prerequisites

**Legal/Business Requirements:**
- Legal entity (LLC, Corporation, registered business)
- DUNS number or equivalent business identifier
- Secretary of State registration
- Physical business address (no P.O. boxes)
- Working business phone number (verification call required)
- Business email domain (must match business registration)

**Technical Requirements:**
- Windows 10/11 machine for token drivers
- USB port for hardware token (or cloud HSM credentials)
- Timestamp server access (internet connectivity)
- signtool.exe (included with Windows SDK)

---

## 2. Certificate Providers Comparison

### 2.1 Recommended Providers

| Provider | EV Code Signing Price | Token/HSM | Timestamp URL | Timeline |
|----------|----------------------|-----------|---------------|----------|
| **DigiCert** | $474/year | USB or Cloud | http://timestamp.digicert.com | 3-7 days |
| **Sectigo (Comodo)** | $415/year | USB or Cloud | http://timestamp.sectigo.com | 5-10 days |
| **GlobalSign** | $599/year | USB or Cloud | http://timestamp.globalsign.com | 7-14 days |

**Recommended:** DigiCert (fastest turnaround, best Windows integration)

### 2.2 Token vs. Cloud HSM

| Type | Pros | Cons | Best For |
|------|------|------|----------|
| **USB Token** | Physical possession, no internet required for signing | Can be lost/damaged, one location | Single developer/small team |
| **Cloud HSM** | Remote access, no hardware to manage, backup/redundancy | Requires internet, subscription | Distributed teams, CI/CD |

---

## 3. Acquisition Steps

### 3.1 Preparation Phase (1-2 days)

1. **Gather Documentation:**
   - Business registration documents
   - EIN/Tax ID
   - DUNS number (obtain from Dun & Bradstreet if needed)
   - Articles of incorporation
   - Business license
   - Photo ID of authorized signatory

2. **Prepare Business Identity:**
   - Ensure business phone number is published and answered
   - Verify business address is on state records
   - Set up business email (e.g., admin@yourbusiness.com)

### 3.2 Order Phase (Same day)

1. Visit provider website (e.g., https://www.digicert.com/code-signing)
2. Select "EV Code Signing Certificate"
3. Choose token type (USB or Cloud HSM)
4. Fill in order form:
   - Organization legal name (exact match to state records)
   - Business address
   - Phone number
   - Email
   - DUNS number

### 3.3 Validation Phase (3-10 days)

**Timeline depends on provider and document readiness**

1. **Document Review (1-2 days):**
   - Provider reviews submitted documents
   - May request additional proof

2. **Phone Verification (1 day):**
   - Provider calls business phone number
   - Authorized person must confirm order
   - Verification code provided

3. **Background Check (1-5 days):**
   - Provider verifies business with state records
   - DUNS lookup
   - Address verification

4. **Approval:**
   - Certificate issued
   - USB token shipped (2-3 days) OR Cloud HSM credentials emailed

### 3.4 Installation Phase (1 day)

**For USB Token:**
1. Install token drivers from provider
2. Insert USB token
3. Import certificate (Windows certmgr.msc or provider tool)
4. Set PIN code
5. Test signing with sample binary

**For Cloud HSM:**
1. Download provider's signing client
2. Configure API credentials
3. Test remote signing

---

## 4. Build Integration

### 4.1 CMake Integration

**Add to CMakeLists.txt:**
```cmake
# Code signing option (OFF by default for dev builds)
option(SIGN_BUILD "Sign binaries with code signing certificate" OFF)

# Code signing certificate thumbprint (from environment)
set(CODE_SIGN_THUMBPRINT $ENV{CODE_SIGN_THUMBPRINT} CACHE STRING "Certificate thumbprint for signing")

# Custom target for signing (runs after build)
if(SIGN_BUILD AND CODE_SIGN_THUMBPRINT)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/scripts/sign.ps1" 
            -BinaryPath "$<TARGET_FILE:${PROJECT_NAME}>" 
            -Thumbprint "${CODE_SIGN_THUMBPRINT}"
        COMMENT "Signing binary with code signing certificate"
    )
endif()
```

### 4.2 Signing Script (scripts/sign.ps1)

```powershell
param(
    [string]$BinaryPath,
    [string]$Thumbprint,
    [string]$TimestampUrl = "http://timestamp.digicert.com"
)

# Find signtool (part of Windows SDK)
$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"

if (-not (Test-Path $signtool)) {
    Write-Error "signtool.exe not found. Install Windows SDK."
    exit 1
}

# Sign the binary
& $signtool sign /tr $TimestampUrl /td sha256 /fd sha256 /sha1 $Thumbprint $BinaryPath

if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Successfully signed: $BinaryPath"
} else {
    Write-Error "❌ Signing failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
}

# Verify signature
& $signtool verify /pa $BinaryPath
```

### 4.3 CI/CD Integration (GitHub Actions Example)

```yaml
name: Build and Sign

on:
  push:
    branches: [main, release/*]

jobs:
  build-and-sign:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Configure CMake
        run: cmake -S . -B build -DSIGN_BUILD=ON -A x64
        env:
          CODE_SIGN_THUMBPRINT: ${{ secrets.CODE_SIGN_THUMBPRINT }}
      
      - name: Build
        run: cmake --build build --config Release
      
      - name: Verify Signature
        run: |
          $binary = "build\bin\Release\RainmeterManager.exe"
          signtool verify /pa $binary
```

---

## 5. Signing Workflow

### 5.1 Local Development (Unsigned)

```bash
# Standard build (unsigned)
cmake -S . -B build -A x64
cmake --build build --config Release
```

### 5.2 Release Build (Signed)

```bash
# Set certificate thumbprint (find via certmgr.msc or PowerShell)
$env:CODE_SIGN_THUMBPRINT="1234567890ABCDEF1234567890ABCDEF12345678"

# Build with signing
cmake -S . -B build -DSIGN_BUILD=ON -A x64
cmake --build build --config Release

# Verify
signtool verify /pa build\bin\Release\RainmeterManager.exe
```

### 5.3 Batch Signing (Multiple Files)

```powershell
# Sign all release binaries
$files = @(
    "build\bin\Release\RainmeterManager.exe",
    "build\bin\Release\RainmeterPlugin.dll",
    "installer\RainmeterManager-Setup.exe"
)

foreach ($file in $files) {
    & scripts\sign.ps1 -BinaryPath $file -Thumbprint $env:CODE_SIGN_THUMBPRINT
}
```

---

## 6. Maintenance and Key Management

### 6.1 Token Security (USB)

**Best Practices:**
- Store token in secure location (safe, locked drawer)
- Never leave token unattended when inserted
- Use strong PIN (6-8 characters, alphanumeric)
- Limit PIN attempts (usually 3-5 before lockout)
- Keep backup token if critical (order 2 from provider)

**If Token is Lost:**
1. Immediately revoke certificate via provider portal
2. Order replacement token with new certificate
3. Update builds with new thumbprint

### 6.2 Certificate Renewal

**Timeline:** Start 30 days before expiration

1. Provider sends renewal notice (usually email)
2. Re-validation may be lighter (business still exists check)
3. New certificate issued (often same token, new cert)
4. Update `CODE_SIGN_THUMBPRINT` in CI/CD secrets
5. Test signing with new cert before old expires

### 6.3 Timestamp Server Importance

**Why Timestamping Matters:**
- Signatures remain valid after certificate expires
- Proves signing occurred when cert was valid
- Without timestamp, signature invalid after cert expiration

**Always use:**
```
/tr http://timestamp.digicert.com /td sha256
```

---

## 7. Troubleshooting

### 7.1 Common Issues

| Error | Cause | Solution |
|-------|-------|----------|
| "Certificate not found" | Thumbprint wrong or cert not imported | Verify thumbprint in certmgr.msc |
| "Private key not accessible" | Token not inserted or drivers missing | Insert token, install drivers |
| "Timestamp server unavailable" | Network issue or server down | Retry or use alternate timestamp URL |
| "Access denied" | Insufficient permissions | Run as Administrator or check PIN |

### 7.2 Verification Commands

```powershell
# List installed certificates
Get-ChildItem Cert:\CurrentUser\My | Where-Object {$_.EnhancedKeyUsageList -like "*Code Signing*"}

# Get thumbprint
(Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert | Select-Object -First 1).Thumbprint

# Verify signed binary
signtool verify /v /pa RainmeterManager.exe

# Check timestamp
signtool verify /v /t Rainmeter Manager.exe | Select-String "Timestamp"
```

---

## 8. Costs and Timeline Summary

**Total Cost:** $400-$600/year (depending on provider)

**Acquisition Timeline:**
- Preparation: 1-2 days
- Order: Same day
- Validation: 3-10 days
- Shipping (USB): 2-3 days
- **Total: 6-15 days**

**Renewal:** Annual (auto-renewal available)

---

## 9. Checklist

**Before Ordering:**
- [ ] Business entity registered and verified
- [ ] DUNS number obtained
- [ ] Business phone number published
- [ ] Business email set up
- [ ] Documents gathered (incorporation, EIN, license)
- [ ] Budget approved ($400-$600/year)

**After Receiving Certificate:**
- [ ] Token/HSM set up and tested
- [ ] Thumbprint documented and stored securely
- [ ] Test signing performed
- [ ] CMake build system updated
- [ ] CI/CD pipeline configured
- [ ] Signing script tested
- [ ] All release binaries signed and verified

---

**Related Documents:**
- SECURITY_AUDIT.md
- RELEASE_COMPLIANCE_CHECKLIST.md
- Build system documentation

**Support Contacts:**
- DigiCert Support: https://www.digicert.com/support
- Sectigo Support: https://sectigo.com/support
- GlobalSign Support: https://www.globalsign.com/en/support
