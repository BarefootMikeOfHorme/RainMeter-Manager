#include "security_adapter.h"
#include "security.h"  // existing security providers (init/cleanup, hashing, signing)
#include <windows.h>
#include <wincrypt.h>
#include <vector>

#pragma comment(lib, "crypt32.lib")

namespace {

std::string Base64Encode(const BYTE* data, DWORD len) {
    if (!data || len == 0) return std::string();
    DWORD outLen = 0;
    if (!CryptBinaryToStringA(data, len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &outLen)) {
        return std::string();
    }
    std::string out(outLen, '\0');
    if (!CryptBinaryToStringA(data, len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out.data(), &outLen)) {
        return std::string();
    }
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

bool Base64Decode(const std::string& b64, std::vector<BYTE>& out) {
    if (b64.empty()) return false;
    DWORD outLen = 0;
    if (!CryptStringToBinaryA(b64.c_str(), static_cast<DWORD>(b64.size()), CRYPT_STRING_BASE64, nullptr, &outLen, nullptr, nullptr)) {
        return false;
    }
    out.resize(outLen);
    return CryptStringToBinaryA(b64.c_str(), static_cast<DWORD>(b64.size()), CRYPT_STRING_BASE64, out.data(), &outLen, nullptr, nullptr) == TRUE;
}

}

namespace RainmeterManager { namespace Core { namespace Security {

bool Initialize() {
    // Initialize underlying BCrypt providers etc.
    return ::Security::initializeCrypto();
}

void Cleanup() {
    ::Security::cleanupCrypto();
}

std::string EncryptString(const std::string& plaintext) {
    DATA_BLOB in { 0 }, out { 0 };
    in.cbData = static_cast<DWORD>(plaintext.size());
    in.pbData = (in.cbData == 0) ? nullptr : (BYTE*)plaintext.data();

    if (!CryptProtectData(&in, L"RainmeterManager", nullptr, nullptr, nullptr, 0, &out)) {
        return std::string();
    }

    std::string b64 = Base64Encode(out.pbData, out.cbData);
    if (out.pbData) LocalFree(out.pbData);
    return b64;
}

std::string DecryptString(const std::string& ciphertext) {
    std::vector<BYTE> enc;
    if (!Base64Decode(ciphertext, enc)) {
        return std::string();
    }
    DATA_BLOB in { 0 }, out { 0 };
    in.cbData = static_cast<DWORD>(enc.size());
    in.pbData = enc.empty() ? nullptr : enc.data();

    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)) {
        return std::string();
    }

    std::string plain;
    if (out.cbData) plain.assign(reinterpret_cast<char*>(out.pbData), reinterpret_cast<char*>(out.pbData) + out.cbData);
    if (out.pbData) LocalFree(out.pbData);
    return plain;
}

}}}
