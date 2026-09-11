#include "FirebaseLogs.h"
#include "FirebaseAuth.h"

#include <windows.h>
#include <winhttp.h>
#include <iphlpapi.h>
#include <bcrypt.h>

#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <stdexcept>

#include "nlohmann/json.hpp"
#include "ThemidaSDK.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "bcrypt.lib")

namespace
{

    constexpr long long kLicenseDurationMs =
        30LL * 24 * 60 * 60 * 1000; // 30 days

    long long CurrentTimeMs()
    {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;

        // FILETIME = 100-ns intervals since 1601-01-01.
        // Convert to Unix-epoch milliseconds.
        constexpr long long kEpochDiffMs = 11644473600000LL;
        return static_cast<long long>(uli.QuadPart / 10000) - kEpochDiffMs;
    }

    using json = nlohmann::json;

    std::wstring Utf8ToWide(const std::string& value)
    {
        if (value.empty())
            return {};

        const int size = MultiByteToWideChar(
            CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0
        );

        if (size <= 0)
            return {};

        std::wstring result(size, L'\0');

        MultiByteToWideChar(
            CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size
        );

        return result;
    }

    std::string WideToUtf8(const std::wstring& value)
    {
        if (value.empty())
            return {};

        const int size = WideCharToMultiByte(
            CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr
        );

        if (size <= 0)
            return {};

        std::string result(size, '\0');

        WideCharToMultiByte(
            CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr
        );

        return result;
    }

    std::string GetLastErrorString()
    {
        const DWORD error = GetLastError();

        if (error == ERROR_SUCCESS)
            return {};

        LPWSTR buffer = nullptr;

        const DWORD length = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error, 0,
            reinterpret_cast<LPWSTR>(&buffer), 0, nullptr
        );

        std::string result;

        if (length && buffer)
        {
            result = WideToUtf8(std::wstring(buffer, length));
            LocalFree(buffer);
        }

        if (result.empty())
            result = "WinHTTP error: " + std::to_string(error);

        return result;
    }

    std::string ReadMachineGuid()
    {
        char buffer[64]{};
        DWORD size = sizeof(buffer);

        const LSTATUS status = RegGetValueA(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Cryptography",
            "MachineGuid",
            RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
            nullptr,
            buffer,
            &size
        );

        if (status != ERROR_SUCCESS)
            return {};

        return std::string(buffer);
    }

    std::string ReadSystemVolumeSerial()
    {
        char sysDir[MAX_PATH]{};

        if (!GetSystemDirectoryA(sysDir, MAX_PATH))
            return {};

        // "C:\Windows\System32" -> "C:\"
        std::string root(sysDir, 3);

        DWORD serial = 0;

        if (!GetVolumeInformationA(
            root.c_str(), nullptr, 0, &serial, nullptr, nullptr, nullptr, 0))
        {
            return {};
        }

        std::ostringstream oss;
        oss << std::hex << serial;
        return oss.str();
    }

    std::string ReadPrimaryMacAddress()
    {
        ULONG size = 0;
        GetAdaptersInfo(nullptr, &size);

        if (size == 0)
            return {};

        std::vector<BYTE> buffer(size);
        auto* adapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());

        if (GetAdaptersInfo(adapterInfo, &size) != NO_ERROR)
            return {};

        std::ostringstream oss;

        for (UINT i = 0; i < adapterInfo->AddressLength; ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(adapterInfo->Address[i]);
        }

        return oss.str();
    }

    std::vector<BYTE> Sha256(const std::string& data)
    {
        std::vector<BYTE> hash;

        BCRYPT_ALG_HANDLE alg = nullptr;
        if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
            return hash;

        DWORD hashObjectSize = 0, hashLength = 0, cbData = 0;

        BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&hashObjectSize), sizeof(DWORD), &cbData, 0);

        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(&hashLength), sizeof(DWORD), &cbData, 0);

        std::vector<BYTE> hashObject(hashObjectSize);
        hash.resize(hashLength);

        BCRYPT_HASH_HANDLE hashHandle = nullptr;

        if (BCryptCreateHash(alg, &hashHandle, hashObject.data(), hashObjectSize, nullptr, 0, 0) == 0)
        {
            BCryptHashData(
                hashHandle,
                reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())),
                static_cast<ULONG>(data.size()),
                0
            );

            BCryptFinishHash(hashHandle, hash.data(), hashLength, 0);
            BCryptDestroyHash(hashHandle);
        }
        else
        {
            hash.clear();
        }

        BCryptCloseAlgorithmProvider(alg, 0);
        return hash;
    }
}

FirebaseLogs::FirebaseLogs(std::string databaseUrl)
    : m_databaseUrl(std::move(databaseUrl))
{
    if (m_databaseUrl.empty())
        throw std::invalid_argument(
            "Firebase database URL cannot be empty."
        );

    while (!m_databaseUrl.empty() && m_databaseUrl.back() == '/')
        m_databaseUrl.pop_back();
}

std::string FirebaseLogs::GetHardwareId()
{
    const std::string raw =
        ReadMachineGuid() + "|" +
        ReadSystemVolumeSerial() + "|" +
        ReadPrimaryMacAddress();

    if (raw == "||")
        return {};

    const std::vector<BYTE> hash = Sha256(raw);

    if (hash.empty())
        return {};

    // Use the first 8 bytes (64 bits) — plenty for account-sharing
    // detection.
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');

    for (int i = 0; i < 8; ++i)
    {
        oss << std::setw(2) << static_cast<int>(hash[i]);
        if (i % 2 == 1 && i != 7)
            oss << '-';
    }

    return oss.str();
}

std::string FirebaseLogs::GetDummyHardwareId()
{
    return "7DEF-C9FA-E613-9979";
}

std::string FirebaseLogs::GetPublicIpAddress()
{
    const Result result = Request(
        "https://api.ipify.org?format=text",
        "",
        L"GET"
    );

    if (!result.success)
        return {};

    std::string ip = result.rawResponse;

    while (!ip.empty() &&
        (ip.back() == '\r' || ip.back() == '\n' || ip.back() == ' '))
    {
        ip.pop_back();
    }

    return ip;
}

std::string FirebaseLogs::SanitizeEmailKey(const std::string& email)
{
    std::string key = email;

    for (char& c : key)
    {
        if (c == '.')
            c = ',';
    }

    return key;
}

std::string FirebaseLogs::FetchStoredHardwareId(
    const std::string& emailKey,
    const std::string& idToken)
{
    const std::string url =
        m_databaseUrl + "/Users/" + emailKey + "/HardwareID.json"
        "?auth=" + FirebaseAuth::UrlEncode(idToken);

    const Result result = Request(url, "", L"GET");

    if (!result.success)
        return {};

    try
    {
        // RTDB returns a bare JSON string (e.g. "\"ABCD-1234\"") for a
        // single-field .json fetch, or the literal "null" if it doesn't
        // exist yet.
        const json value = json::parse(result.rawResponse);

        if (value.is_string())
            return value.get<std::string>();
    }
    catch (const std::exception&)
    {
        // Not valid JSON / unexpected shape -- treat as "no prior record".
    }

    return {};
}

FirebaseLogs::Result FirebaseLogs::RecordLogin(
    const std::string& email,
    const std::string& idToken,
    const std::string& hardwareId,
    const std::string& ipAddress)
{
    if (email.empty() || idToken.empty())
    {
        Result result;
        result.error = "Cannot record login: missing email or ID token.";
        return result;
    }

    const std::string emailKey = SanitizeEmailKey(email);

    // Check the previously recorded hardware ID before we overwrite it.
    const std::string previousHardwareId =
        FetchStoredHardwareId(emailKey, idToken);

    const bool flaggedSharing =
        !previousHardwareId.empty() &&
        previousHardwareId != hardwareId;

    const std::string timestampKey = std::to_string(
        static_cast<long long>(time(nullptr))
    );

    json payload =
    {
        {"Users/" + emailKey + "/HardwareID", hardwareId},
        {"Users/" + emailKey + "/IPAddress", ipAddress},
        {"Users/" + emailKey + "/LastLogin", {{".sv", "timestamp"}}},
        {"Users/" + emailKey + "/LoginHistory/" + timestampKey, {
            {"HardwareID", hardwareId},
            {"IPAddress", ipAddress},
            {"Timestamp", {{".sv", "timestamp"}}}
        }}
    };

    if (flaggedSharing)
    {
        payload["SuspiciousAccounts/" + emailKey + "/SuspiciousForAccountSharing"] = true;
    }

    const std::string url =
        m_databaseUrl + "/.json?auth=" + FirebaseAuth::UrlEncode(idToken);

    Result result = Request(url, payload.dump(), L"PATCH");
    result.flaggedSharing = flaggedSharing;

    return result;
}

FirebaseLogs::Result FirebaseLogs::Request(
    const std::string& url,
    const std::string& body,
    const std::wstring& verb)
{
    Result result;

    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);

    wchar_t hostName[256]{};
    wchar_t urlPath[4096]{};
    wchar_t extraInfo[4096]{};

    components.lpszHostName = hostName;
    components.dwHostNameLength = static_cast<DWORD>(std::size(hostName));

    components.lpszUrlPath = urlPath;
    components.dwUrlPathLength = static_cast<DWORD>(std::size(urlPath));

    components.lpszExtraInfo = extraInfo;
    components.dwExtraInfoLength = static_cast<DWORD>(std::size(extraInfo));

    const std::wstring wideUrl = Utf8ToWide(url);

    if (!WinHttpCrackUrl(
        wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &components))
    {
        result.error = GetLastErrorString();
        return result;
    }

    HINTERNET session = WinHttpOpen(
        L"FirebaseLogs/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session)
    {
        result.error = GetLastErrorString();
        return result;
    }

    DWORD secureProtocols =
        WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 |
        WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;

    WinHttpSetOption(
        session, WINHTTP_OPTION_SECURE_PROTOCOLS,
        &secureProtocols, sizeof(secureProtocols)
    );

    WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);

    HINTERNET connection = WinHttpConnect(
        session, components.lpszHostName, components.nPort, 0
    );

    if (!connection)
    {
        result.error = GetLastErrorString();
        WinHttpCloseHandle(session);
        return result;
    }

    std::wstring requestPath = components.lpszUrlPath;

    if (components.lpszExtraInfo && components.dwExtraInfoLength > 0)
        requestPath += components.lpszExtraInfo;

    DWORD flags = 0;

    if (components.nScheme == INTERNET_SCHEME_HTTPS)
        flags |= WINHTTP_FLAG_SECURE;

    HINTERNET request = WinHttpOpenRequest(
        connection,
        verb.c_str(),
        requestPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!request)
    {
        result.error = GetLastErrorString();
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return result;
    }

    const wchar_t* headers =
        L"Content-Type: application/json; charset=utf-8\r\n"
        L"Accept: application/json\r\n";

    if (!WinHttpSendRequest(
        request,
        headers,
        static_cast<DWORD>(-1L),
        const_cast<char*>(body.data()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0))
    {
        result.error = GetLastErrorString();
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return result;
    }

    if (!WinHttpReceiveResponse(request, nullptr))
    {
        result.error = GetLastErrorString();
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return result;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);

    WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusSize,
        WINHTTP_NO_HEADER_INDEX
    );

    result.httpStatus = static_cast<long>(statusCode);

    std::string response;

    for (;;)
    {
        DWORD available = 0;

        if (!WinHttpQueryDataAvailable(request, &available))
        {
            result.error = GetLastErrorString();
            break;
        }

        if (available == 0)
            break;

        std::vector<char> buffer(available);
        DWORD downloaded = 0;

        if (!WinHttpReadData(request, buffer.data(), available, &downloaded))
        {
            result.error = GetLastErrorString();
            break;
        }

        response.append(buffer.data(), downloaded);
    }

    result.rawResponse = std::move(response);

    result.success =
        result.error.empty() &&
        statusCode >= 200 &&
        statusCode < 300;

    if (!result.success && result.error.empty())
    {
        result.error = "Firebase Logs HTTP error " + std::to_string(statusCode);

        if (!result.rawResponse.empty())
            result.error += ": " + result.rawResponse;
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return result;
}

long long FirebaseLogs::FetchLicenseExpiry(
    const std::string& emailKey,
    const std::string& idToken,
    bool& exists)
{
    exists = false;

    const std::string url =
        m_databaseUrl + "/Users/" + emailKey + "/LicenseExpiry.json"
        "?auth=" + FirebaseAuth::UrlEncode(idToken);

    const Result result = Request(url, "", L"GET");

    if (!result.success)
        return 0;

    try
    {
        const json value = json::parse(result.rawResponse);

        if (value.is_number())
        {
            exists = true;
            return value.get<long long>();
        }
    }
    catch (const std::exception&)
    {
        // Not valid JSON / unexpected shape -- treat as "no prior record".
    }

    return 0;
}

FirebaseLogs::LicenseResult FirebaseLogs::CheckLicense(
    const std::string& email,
    const std::string& idToken)
{
    VM_START

    LicenseResult license;

    if (email.empty() || idToken.empty())
    {
        license.error = "Cannot check license: missing email or ID token.";
        return license;
    }

    const std::string emailKey = SanitizeEmailKey(email);

    bool exists = false;
    const long long storedExpiry =
        FetchLicenseExpiry(emailKey, idToken, exists);

    const long long now = CurrentTimeMs();

    if (!exists)
    {

        

        // First login for this user -- activate a fresh 30-day license.
        const long long expiresAt = now + kLicenseDurationMs;

        const json payload =
        {
            {"LicenseExpiry", expiresAt}
        };

        const std::string url =
            m_databaseUrl + "/Users/" + emailKey + ".json"
            "?auth=" + FirebaseAuth::UrlEncode(idToken);

        const Result writeResult = Request(url, payload.dump(), L"PATCH");

        if (!writeResult.success)
        {
            license.status = LicenseStatus::Error;
            license.error = writeResult.error;
            return license;
        }

        //license.status = LicenseStatus::Active;
        //license.expiresAt = expiresAt;

       
            license.status =
            (now >= storedExpiry) ? LicenseStatus::Expired : LicenseStatus::Active;
       

        return license;
    }

    license.expiresAt = storedExpiry;
    license.status =
        (now >= storedExpiry) ? LicenseStatus::Expired : LicenseStatus::Active;

    VM_END

    return license;
}

bool FirebaseLogs::IsAccountDisabled(
    const std::string& email,
    const std::string& idToken)
{
    VM_START

    const std::string emailKey = SanitizeEmailKey(email);

    const std::string url =
        m_databaseUrl + "/Users/" + emailKey + "/Disabled.json"
        "?auth=" + FirebaseAuth::UrlEncode(idToken);

    const Result result = Request(url, "", L"GET");

    bool disabled = false;

    if (result.success)
    {
        try
        {
            const json value = json::parse(result.rawResponse);

            if (value.is_boolean())
                disabled = value.get<bool>();
        }
        catch (const std::exception&)
        {
            // Not valid JSON / unexpected shape -- treat as "not disabled".
        }
    }


        const bool finalResult = disabled;

    VM_END

        return finalResult;
}