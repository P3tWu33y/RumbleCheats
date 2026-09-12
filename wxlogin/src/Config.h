#pragma once

// Fill these in with your own Firebase project details before building.
//
//  - kFirebaseApiKey     -> Project settings -> General -> Web API Key
//  - kFirebaseDatabaseUrl-> Realtime Database -> the "https://xxx.firebaseio.com" URL
//                           (only needed if/when the tool talks to the DB
//                           via FirebaseCRUD, e.g. for license/user records)

namespace Config
{
    constexpr const char* kFirebaseApiKey      = "AIzaSyAIv9Ui5FpDc9xlEsCBoBMtmu1iI7mHF40";
    constexpr const char* kFirebaseDatabaseUrl = "https://rumblefighter-39d77-default-rtdb.europe-west1.firebasedatabase.app";
    constexpr const char* kAppVersion = "1.0.1";

    // Small branding line shown on the Login and Tool windows.
    constexpr const char* kDiscordAd = ".petwussy - Discord";

    constexpr const char* kAppTitle = "Wussy's Tool";

    // Process the Tool window looks for.
    constexpr const wchar_t* kTargetProcessName = L"RumbleFighter.exe";
 
    const std::string version = "v1.0.0-RF";
    const std::string assetName = "module.dll";

}


