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

                if (GetTempPathW(MAX_PATH, tempPath) == 0)
                {
                    wxMessageBox(
                        "Failed to get temporary directory.",
                        "Update Failed",
                        wxOK | wxICON_ERROR
                    );

                    return false;
                }

                const std::wstring downloadPath =
                    std::wstring(tempPath) + L"ClientUpdate.exe";

                wxLogDebug(
                    "Update URL: %s",
                    versionInfo.downloadUrl
                );

                wxLogDebug(
                    "Update path: %ls",
                    downloadPath.c_str()
                );

                const bool downloaded =
                    updater.DownloadFile(
                        versionInfo.downloadUrl,
                        downloadPath
                    );

                if (!downloaded)
                {
                    wxLogDebug("DownloadFile FAILED");

                    wxMessageBox(
                        "The update could not be downloaded.",
                        "Update Failed",
                        wxOK | wxICON_ERROR
                    );

                    return false;
                }

                wxLogDebug("DownloadFile SUCCEEDED");

                wchar_t currentExePath[MAX_PATH]{};

                if (GetModuleFileNameW(
                    nullptr,
                    currentExePath,
                    MAX_PATH) == 0)
                {
                    wxMessageBox(
                        "Failed to determine the current executable path.",
                        "Update Failed",
                        wxOK | wxICON_ERROR
                    );

                    return false;
                }

                wxLogDebug(
                    "Current EXE: %ls",
                    currentExePath
                );

                const bool launched =
                    updater.LaunchUpdaterAndExit(
                        downloadPath,
                        currentExePath
                    );

                if (!launched)
                {
                    wxLogDebug("LaunchUpdaterAndExit FAILED");

                    wxMessageBox(
                        "The update was downloaded, but the updater could not be launched.",
                        "Update Failed",
                        wxOK | wxICON_ERROR
                    );

                    return false;
                }

                // Updater was successfully launched.
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