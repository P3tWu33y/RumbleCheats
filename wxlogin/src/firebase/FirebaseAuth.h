#pragma once

#include <string>

#include "nlohmann/json.hpp"

class FirebaseAuth
{
public:
    using json = nlohmann::json;

    struct Result
    {
        bool success = false;
        long httpStatus = 0;
        std::string rawResponse;
        std::string error;
    };

    explicit FirebaseAuth(std::string apiKey);
    ~FirebaseAuth();

    // Logs in with an email + password against Firebase Identity Toolkit.
    // On success, GetIdToken()/GetUid()/GetEmail() become populated.
    Result Login(const std::string& email, const std::string& password);

    const std::string& GetIdToken() const;
    const std::string& GetRefreshToken() const;
    const std::string& GetUid() const;
    const std::string& GetEmail() const;

    bool IsAuthenticated() const;
    void Logout();

    static std::string UrlEncode(const std::string& value);

private:
    Result Request(const std::string& url, const std::string& body);
    static void SecureWipe(std::string& value);

    std::string m_apiKey;
    std::string m_idToken;
    std::string m_refreshToken;
    std::string m_uid;
    std::string m_email;
};