#include "Firebase.h"

#include <windows.h>
#include <winhttp.h>

#include <sstream>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace
{
    std::wstring Utf8ToWide(const std::string& value)
    {
        if (value.empty())
            return {};

        const int size = MultiByteToWideChar(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0
        );

        if (size <= 0)
            return {};

        std::wstring result(size, L'\0');

        MultiByteToWideChar(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            size
        );

        return result;
    }

    std::string WideToUtf8(const std::wstring& value)
    {
        if (value.empty())
            return {};

        const int size = WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0,
            nullptr,
            nullptr
        );

        if (size <= 0)
            return {};

        std::string result(size, '\0');

        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            size,
            nullptr,
            nullptr
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
            nullptr,
            error,
            0,
            reinterpret_cast<LPWSTR>(&buffer),
            0,
            nullptr
        );

        std::string result;

        if (length && buffer)
        {
            result = WideToUtf8(std::wstring(buffer, length));
            LocalFree(buffer);
        }

        if (result.empty())
        {
            result = "WinHTTP error: " + std::to_string(error);
        }

        return result;
    }
}

FirebaseCRUD::FirebaseCRUD(std::string databaseUrl,
                           std::string authToken)
    : m_databaseUrl(std::move(databaseUrl)),
      m_authToken(std::move(authToken))
{
    while (!m_databaseUrl.empty() && m_databaseUrl.back() == '/')
        m_databaseUrl.pop_back();

    if (m_databaseUrl.empty())
        throw std::invalid_argument("Firebase database URL cannot be empty.");
}

void FirebaseCRUD::SetDatabaseUrl(const std::string& databaseUrl)
{
    m_databaseUrl = databaseUrl;

    while (!m_databaseUrl.empty() && m_databaseUrl.back() == '/')
        m_databaseUrl.pop_back();
}

void FirebaseCRUD::SetAuthToken(const std::string& authToken)
{
    m_authToken = authToken;
}

const std::string& FirebaseCRUD::GetDatabaseUrl() const
{
    return m_databaseUrl;
}

const std::string& FirebaseCRUD::GetAuthToken() const
{
    return m_authToken;
}

std::string FirebaseCRUD::NormalizePath(const std::string& path)
{
    std::string result = path;

    while (!result.empty() && result.front() == '/')
        result.erase(result.begin());

    while (!result.empty() && result.back() == '/')
        result.pop_back();

    return result;
}

std::string FirebaseCRUD::BuildUrl(const std::string& path) const
{
    std::string url = m_databaseUrl;

    if (!url.empty() && url.back() != '/')
        url += '/';

    url += NormalizePath(path);

    if (url.size() < 5 ||
        url.compare(url.size() - 5, 5, ".json") != 0)
    {
        url += ".json";
    }

    if (!m_authToken.empty())
    {
        url += "?auth=";
        url += UrlEncode(m_authToken);
    }

    return url;
}

std::string FirebaseCRUD::UrlEncode(const std::string& value)
{
    static constexpr char hex[] = "0123456789ABCDEF";

    std::string result;

    for (unsigned char c : value)
    {
        const bool safe =
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' ||
            c == '_' ||
            c == '.' ||
            c == '~';

        if (safe)
        {
            result += static_cast<char>(c);
        }
        else
        {
            result += '%';
            result += hex[(c >> 4) & 0x0F];
            result += hex[c & 0x0F];
        }
    }

    return result;
}

FirebaseCRUD::Result FirebaseCRUD::Request(
    const std::string& method,
    const std::string& path,
    std::optional<json> body) const
{
    std::string bodyString;

    if (body.has_value())
        bodyString = body->dump();

    return RequestInternal(
        method,
        BuildUrl(path),
        bodyString
    );
}

FirebaseCRUD::Result FirebaseCRUD::RequestInternal(
    const std::string& method,
    const std::string& url,
    const std::string& body) const
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
            wideUrl.c_str(),
            static_cast<DWORD>(wideUrl.size()),
            0,
            &components))
    {
        result.error = GetLastErrorString();
        return result;
    }

    HINTERNET session = WinHttpOpen(
        L"FirebaseCRUD/1.0",
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

    HINTERNET connection = WinHttpConnect(
        session,
        components.lpszHostName,
        components.nPort,
        0
    );

    if (!connection)
    {
        result.error = GetLastErrorString();
        WinHttpCloseHandle(session);
        return result;
    }

    std::wstring requestPath = components.lpszUrlPath;

    if (components.lpszExtraInfo &&
        components.dwExtraInfoLength > 0)
    {
        requestPath += components.lpszExtraInfo;
    }

    DWORD flags = 0;

    if (components.nScheme == INTERNET_SCHEME_HTTPS)
        flags |= WINHTTP_FLAG_SECURE;

    const std::wstring wideMethod = Utf8ToWide(method);

    HINTERNET request = WinHttpOpenRequest(
        connection,
        wideMethod.c_str(),
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

    const void* requestData =
        body.empty()
            ? WINHTTP_NO_REQUEST_DATA
            : static_cast<const void*>(body.data());

    const DWORD requestDataLength =
        static_cast<DWORD>(body.size());

    if (!WinHttpSendRequest(
            request,
            headers,
            static_cast<DWORD>(-1L),
            const_cast<void*>(requestData),
            requestDataLength,
            requestDataLength,
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
        WINHTTP_QUERY_STATUS_CODE |
        WINHTTP_QUERY_FLAG_NUMBER,
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

        if (!WinHttpQueryDataAvailable(
                request,
                &available))
        {
            result.error = GetLastErrorString();
            break;
        }

        if (available == 0)
            break;

        std::vector<char> buffer(available);

        DWORD downloaded = 0;

        if (!WinHttpReadData(
                request,
                buffer.data(),
                available,
                &downloaded))
        {
            result.error = GetLastErrorString();
            break;
        }

        response.append(buffer.data(), downloaded);
    }

    result.body = std::move(response);

    result.success =
        result.error.empty() &&
        statusCode >= 200 &&
        statusCode < 300;

    if (!result.success && result.error.empty())
    {
        result.error =
            "Firebase HTTP error " +
            std::to_string(statusCode);

        if (!result.body.empty())
            result.error += ": " + result.body;
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return result;
}

std::optional<FirebaseCRUD::json> FirebaseCRUD::Get(
    const std::string& path,
    std::string* error) const
{
    Result result = Request("GET", path);

    if (!result.success)
    {
        if (error)
            *error = result.error;

        return std::nullopt;
    }

    try
    {
        return json::parse(result.body);
    }
    catch (const std::exception& e)
    {
        if (error)
            *error = std::string("Invalid JSON returned by Firebase: ") +
                     e.what();

        return std::nullopt;
    }
}

FirebaseCRUD::Result FirebaseCRUD::Create(
    const std::string& path,
    const json& data) const
{
    return Request("PUT", path, std::optional<json>{data});
}

FirebaseCRUD::Result FirebaseCRUD::CreateWithGeneratedKey(
    const std::string& collectionPath,
    const json& data) const
{
    return Request("POST", collectionPath, std::optional<json>{data});
}

FirebaseCRUD::Result FirebaseCRUD::Update(
    const std::string& path,
    const json& data) const
{
    return Request("PATCH", path, std::optional<json>{data});
}

FirebaseCRUD::Result FirebaseCRUD::Delete(
    const std::string& path) const
{
    return Request("DELETE", path);
}
