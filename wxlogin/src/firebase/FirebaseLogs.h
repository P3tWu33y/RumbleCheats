#pragma once

#include <string>

class FirebaseLogs
{
public:
    struct Result
    {
        bool success = false;
        long httpStatus = 0;
        std::string rawResponse;
        std::string error;

        // True if this login was flagged as a potential account-sharing
        // case (HardwareID differs from the last recorded one).
        bool flaggedSharing = false;
    };

    enum class LicenseStatus
    {
        Active,
        Expired,
        Error
    };

    struct LicenseResult
    {
        LicenseStatus status = LicenseStatus::Error;
        long long expiresAt = 0; // epoch milliseconds
        std::string error;
    };

    explicit FirebaseLogs(std::string databaseUrl);

    // Builds a stable per-machine identifier from the Windows install GUID,
    // system volume serial, and primary NIC MAC address, hashed with
    // SHA-256. Returns a hyphenated hex string, or empty on failure.
    std::string GetHardwareId();

    // Fixed placeholder ID for testing/development ONLY. Never call this
    // from a release build -- it defeats hardware-based account-sharing
    // detection entirely for whoever uses it.
    std::string GetDummyHardwareId();

    // Fetches this machine's public IP via api.ipify.org. Empty on failure.
    std::string GetPublicIpAddress();

    // Writes HardwareID/IPAddress/LastLogin + a LoginHistory entry for the
    // given user, and flags SuspiciousAccounts/<email>/SuspiciousForAccountSharing
    // if the hardware ID differs from the previously stored one.
    // Call after a successful FirebaseAuth::Login().
    Result RecordLogin(
        const std::string& email,
        const std::string& idToken,
        const std::string& hardwareId,
        const std::string& ipAddress);

    // Checks this user's license. On the very first call for a user (no
    // LicenseExpiry stored yet), activates a fresh 30-day license and
    // returns Active. On later calls, compares against the stored value.
    LicenseResult CheckLicense(
        const std::string& email,
        const std::string& idToken);

    // Returns true if this user's account is marked disabled.
    // On fetch failure, returns false (fails open).
    bool IsAccountDisabled(
        const std::string& email,
        const std::string& idToken);

    static std::string SanitizeEmailKey(const std::string& email);

private:
    Result Request(
        const std::string& url,
        const std::string& body,
        const std::wstring& verb = L"POST");

    // Fetches the previously stored HardwareID for a user, if any.
    // Returns empty string if there's no prior record or the fetch fails.
    std::string FetchStoredHardwareId(
        const std::string& emailKey,
        const std::string& idToken);

    long long FetchLicenseExpiry(
        const std::string& emailKey,
        const std::string& idToken,
        bool& exists);

    std::string m_databaseUrl;
};