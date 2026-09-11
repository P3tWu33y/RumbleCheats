#pragma once

#include <cstdint>
#include <string>
#include <vector>

class AppUpdater
{
public:
    struct VersionInfo
    {
        bool success = false;
        std::string error;
        std::string latestVersion;
        std::string downloadUrl;
        bool forceUpdate = false;
    };

    explicit AppUpdater(std::string databaseUrl);

    // Fetches AppVersion/{Latest,DownloadUrl,ForceUpdate} -- no auth needed,
    // this must be checkable before login.
    VersionInfo CheckForUpdate();

    // Returns true if `remote` is a newer version than `local`.
    // Compares dotted numeric versions (e.g. "1.2.0" vs "1.10.0").
    static bool IsNewerVersion(
        const std::string& local,
        const std::string& remote);

    // Downloads the file at `url` to `destinationPath`. Blocking.
    bool DownloadFile(
        const std::string& url,
        const std::wstring& destinationPath);

    // Downloads the file at `url` directly into memory.
    // Returns an empty vector on failure.
    std::vector<std::uint8_t> DownloadToMemory(const std::string& url, std::string* error = nullptr);

    // Launches Updater.exe with the downloaded file + target exe path,
    // then the caller should exit immediately.
    bool LaunchUpdaterAndExit(
        const std::wstring& downloadedExePath,
        const std::wstring& targetExePath);

private:
    struct Result
    {
        bool success = false;
        long httpStatus = 0;
        std::string rawResponse;
        std::string error;
    };

    Result Request(const std::string& url);

    std::string m_databaseUrl;
};