#pragma once

// FirebaseCRUD
// Thin WinHTTP-based client for the Firebase Realtime Database REST API.
// Implementation lives in Firebase.cpp (as provided).

#include <string>
#include <optional>

#include "nlohmann/json.hpp"

class FirebaseCRUD
{
public:
    using json = nlohmann::json;

    struct Result
    {
        bool success = false;
        long httpStatus = 0;
        std::string body;
        std::string error;
    };

    FirebaseCRUD(std::string databaseUrl, std::string authToken = {});

    void SetDatabaseUrl(const std::string& databaseUrl);
    void SetAuthToken(const std::string& authToken);

    const std::string& GetDatabaseUrl() const;
    const std::string& GetAuthToken() const;

    std::optional<json> Get(const std::string& path, std::string* error = nullptr) const;

    Result Create(const std::string& path, const json& data) const;
    Result CreateWithGeneratedKey(const std::string& collectionPath, const json& data) const;
    Result Update(const std::string& path, const json& data) const;
    Result Delete(const std::string& path) const;

private:
    Result Request(
        const std::string& method,
        const std::string& path,
        std::optional<json> body = std::nullopt
    ) const;

    Result RequestInternal(const std::string& method,
                            const std::string& url,
                            const std::string& body) const;

    static std::string NormalizePath(const std::string& path);
    std::string BuildUrl(const std::string& path) const;
    static std::string UrlEncode(const std::string& value);

    std::string m_databaseUrl;
    std::string m_authToken;
};
