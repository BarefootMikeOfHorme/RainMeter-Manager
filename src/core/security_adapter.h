#pragma once

#include <string>

namespace RainmeterManager {
namespace Core {
namespace Security {

bool Initialize();
void Cleanup();

// Encrypt/decrypt using Windows DPAPI, returned as Base64 strings
std::string EncryptString(const std::string& plaintext);
std::string DecryptString(const std::string& ciphertext);

} // namespace Security
} // namespace Core
} // namespace RainmeterManager

