#include <wx/wx.h>

#include "LoginFrame.h"
#include "AntiDebug.h"
#include "updater\AppUpdater.h"
#include "Config.h"

class WussysToolApp : public wxApp
{
public:
    bool OnInit() override
    {
        if (!wxApp::OnInit())
            return false;

        // ---------------------------------------------------------------
        // Forced update check -- must run before anything else, since a
        // stale client should never reach LoginFrame at all.
        // ---------------------------------------------------------------

        AppUpdater updater(Config::kFirebaseDatabaseUrl);

        const AppUpdater::VersionInfo versionInfo =
            updater.CheckForUpdate();

        bool updateRequired = false;

        VM_START
            updateRequired =
            versionInfo.success &&
            versionInfo.forceUpdate &&
            AppUpdater::IsNewerVersion(Config::kAppVersion, versionInfo.latestVersion);
        VM_END

            if (updateRequired)
            {
                wchar_t tempPath[MAX_PATH]{};
                GetTempPathW(MAX_PATH, tempPath);

                const std::wstring downloadPath =
                    std::wstring(tempPath) + L"ClientUpdate.exe";

                const bool downloaded =
                    updater.DownloadFile(versionInfo.downloadUrl, downloadPath);

                if (downloaded)
                {
                    wchar_t currentExePath[MAX_PATH]{};
                    GetModuleFileNameW(nullptr, currentExePath, MAX_PATH);

                    if (updater.LaunchUpdaterAndExit(downloadPath, currentExePath))
                        return false; // Exit immediately -- Updater.exe takes over.

                    wxLogDebug("LaunchUpdaterAndExit failed");
                }
                else
                {
                    wxLogDebug("DownloadFile failed");
                }

                // A forced update was required but couldn't be downloaded or
                // launched -- refuse to run on the outdated version rather
                // than silently letting the user in.
                wxMessageBox(
                    "A required update could not be installed.\n"
                    "Please check your internet connection and try again.",
                    "Update Failed",
                    wxOK | wxICON_ERROR
                );

                return false;
            }

        LoginFrame* login = new LoginFrame();
        login->Show();
        SetTopWindow(login);

        // Allocate a console for debugging output -- Don't forget to comment it out when you release the program, this is just for testing purposes
        //AllocConsole();
        //FILE* f;
        //freopen_s(&f, "CONOUT$", "w", stdout);

        //std::cout << "Shit man... You just turned on my VIRUS!\n";

        return true;
    }
};

wxIMPLEMENT_APP(WussysToolApp);