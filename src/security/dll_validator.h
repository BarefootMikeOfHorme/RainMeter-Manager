// dll_validator.h - Smart DLL Validation with User Trust Management
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>

namespace RainmeterManager {
namespace Security {

// DLL validation result
enum class ValidationResult {
    Trusted,            // Signed by known publisher or user
    UserSigned,         // Signed by user's own certificate
    CommunityApproved,  // In community whitelist
    KnownSafe,          // Hash matches known-safe DLL
    Unsigned,           // No signature, needs user decision
    ModifiedSafe,       // Modified version of known-safe DLL
    Suspicious,         // Failed checks, recommend blocking
    Blocked             // Explicitly blocked by user/policy
};

// Trust source
enum class TrustSource {
    Microsoft,          // Microsoft-signed
    UserCertificate,    // User's own certificate
    CommunityWhitelist, // Community-approved
    HashWhitelist,      // Known-good hash
    UserApproved,       // User explicitly approved
    SystemDefault       // Default Windows trust
};

// DLL signature info
struct SignatureInfo {
    bool isSigned;
    std::string publisherName;
    std::string issuerName;
    std::string serialNumber;
    std::string thumbprint;
    bool isValid;
    bool isTrusted;
    std::string signatureAlgorithm;
    std::string timestamp;
};

// DLL validation context
struct DLLValidationContext {
    std::string dllPath;
    std::string dllName;
    std::string sha256Hash;
    SignatureInfo signature;
    ValidationResult result;
    TrustSource trustSource;
    std::string riskDescription;
    std::vector<std::string> recommendations;
    bool allowLoad;
};

// User trust decision
struct UserTrustDecision {
    std::string dllHash;
    std::string dllName;
    bool trusted;
    std::string reason;
    std::chrono::system_clock::time_point timestamp;
    bool applyToAllVersions;  // Trust all versions of this DLL
};

// Configuration
struct DLLValidatorConfig {
    bool enableValidation = true;
    bool blockUnsigned = false;           // Don't block by default, just warn
    bool allowUserSigned = true;          // Trust user's own certs
    bool checkCommunityDB = true;         // Check community whitelist
    bool promptOnUnsigned = true;         // Ask user for unsigned DLLs
    bool autoTrustSystemDLLs = true;      // Trust Windows system DLLs
    bool enableHashWhitelist = true;      // Check known-good hashes
    
    std::string userCertThumbprint;       // User's code signing cert
    std::string communityDBPath;          // Community trust database
    std::string hashWhitelistPath;        // Known-good DLL hashes
    std::string userTrustStorePath;       // User's trust decisions
};

/**
 * @brief Smart DLL Validator - Help Users Be Safe and Creative
 * 
 * Philosophy:
 * - Verify signatures and provide risk assessment
 * - Trust user-signed DLLs (encourage signing their work)
 * - Community whitelist for popular plugins
 * - User education over restriction
 * - Remember user decisions
 */
class DLLValidator {
public:
    DLLValidator();
    ~DLLValidator();

    // Configuration
    void Configure(const DLLValidatorConfig& config);
    
    // Main validation
    DLLValidationContext ValidateDLL(const std::string& dllPath);
    bool ShouldAllowLoad(const DLLValidationContext& context);
    
    // Signature verification
    SignatureInfo VerifySignature(const std::string& dllPath);
    bool IsSignedByUser(const SignatureInfo& sig);
    bool IsSignedByTrustedPublisher(const SignatureInfo& sig);
    
    // Hash verification
    std::string ComputeSHA256(const std::string& filePath);
    bool IsHashWhitelisted(const std::string& hash);
    bool IsCommunityApproved(const std::string& dllName, const std::string& hash);
    
    // User trust management
    void RecordUserDecision(const UserTrustDecision& decision);
    bool HasUserApproved(const std::string& dllHash);
    UserTrustDecision GetUserDecision(const std::string& dllHash);
    std::vector<UserTrustDecision> GetAllUserDecisions();
    void RemoveUserDecision(const std::string& dllHash);
    
    // Certificate management
    bool SetUserCertificate(const std::string& thumbprint);
    std::string GetUserCertificate() const;
    std::vector<std::string> GetInstalledCodeSigningCerts();
    
    // Community database
    void UpdateCommunityDatabase();
    void AddToCommunityWhitelist(const std::string& dllName, const std::string& hash, const std::string& description);
    
    // Hash whitelist
    void AddToHashWhitelist(const std::string& hash, const std::string& description);
    void RemoveFromHashWhitelist(const std::string& hash);
    
    // Helpers for users
    std::string GetRiskAssessment(const DLLValidationContext& context);
    std::vector<std::string> GetSecurityRecommendations(const DLLValidationContext& context);
    std::string GetSigningInstructions();  // How to sign their own DLLs
    
    // Statistics
    struct Statistics {
        uint64_t dllsValidated;
        uint64_t signedDLLs;
        uint64_t unsignedDLLs;
        uint64_t userSignedDLLs;
        uint64_t communityApprovedDLLs;
        uint64_t blockedDLLs;
        uint64_t userApprovedDLLs;
    };
    Statistics GetStatistics() const;

private:
    DLLValidatorConfig config_;
    mutable std::mutex mutex_;
    
    // Trust stores
    std::unordered_map<std::string, UserTrustDecision> userTrustStore_;      // hash -> decision
    std::unordered_map<std::string, std::string> hashWhitelist_;              // hash -> description
    std::unordered_map<std::string, std::unordered_set<std::string>> communityDB_;  // dllName -> hashes
    
    // Statistics
    Statistics stats_;
    mutable std::mutex statsMutex_;
    
    // Internal methods
    bool CheckWindowsSignature(const std::string& filePath, SignatureInfo& info);
    ValidationResult DetermineValidationResult(const DLLValidationContext& context);
    bool IsSystemDLL(const std::string& dllPath);
    void LoadUserTrustStore();
    void SaveUserTrustStore();
    void LoadHashWhitelist();
    void LoadCommunityDatabase();
    std::string ExtractCertificateInfo(PCCERT_CONTEXT certContext, const std::string& field);
};

/**
 * @brief DLL Signing Helper - Tools to Help Users Sign Their Work
 */
class DLLSigningHelper {
public:
    // Check if user has code signing certificate
    static bool HasCodeSigningCertificate();
    
    // Create self-signed certificate for development
    static bool CreateDevelopmentCertificate(const std::string& subjectName);
    
    // Sign a DLL with user's certificate
    static bool SignDLL(const std::string& dllPath, const std::string& certThumbprint);
    
    // Verify DLL signature
    static bool VerifyDLLSignature(const std::string& dllPath);
    
    // Get instructions for users
    static std::string GetSigningGuide();
    static std::string GetCertificateCreationGuide();
    
    // Tools integration
    static std::string GetSignToolPath();  // Find signtool.exe
    static bool IsSignToolAvailable();
};

} // namespace Security
} // namespace RainmeterManager
