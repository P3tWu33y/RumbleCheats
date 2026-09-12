#include "LoginFrame.h"

#include "Theme.h"
#include "Config.h"
#include "ToolFrame.h"
#include "ThemidaSDK.h"
#include "AppUpdater.h"

namespace
{
    constexpr int ID_UsernameCtrl = wxID_HIGHEST + 1;
    constexpr int ID_PasswordCtrl = wxID_HIGHEST + 2;
    constexpr int ID_LoginButton = wxID_HIGHEST + 3;
}

LoginFrame::LoginFrame()
    : wxFrame(
        nullptr,
        wxID_ANY,
        Config::kAppTitle,
        wxDefaultPosition,
        wxSize(380, 420),
        wxDEFAULT_FRAME_STYLE &
        ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)
    )
    , m_auth(
        std::make_unique<FirebaseAuth>(
            Config::kFirebaseApiKey
        )
    )
    , m_logs(
        std::make_unique<FirebaseLogs>(
            Config::kFirebaseDatabaseUrl
        )
    )
{
    BuildUi();
    CentreOnScreen();

    // Lock the form immediately -- the user shouldn't be able to attempt
    // a login before we know the client itself is current. The actual
    // check is deferred with CallAfter() so it runs once the event loop
    // is pumping (the frame isn't shown/mapped yet inside the constructor).
    SetBusy(true);
    m_loginButton->SetLabel("Checking for updates...");

    CallAfter(&LoginFrame::EnsureClientUpToDate);
}

void LoginFrame::BuildUi()
{
    Theme::ApplyWindowTheme(this);

    wxPanel* root = new wxPanel(this);
    Theme::ApplyWindowTheme(root);

    wxBoxSizer* rootSizer =
        new wxBoxSizer(wxVERTICAL);

    rootSizer->AddStretchSpacer();

    // ---------------------------------------------------------------------
    // Title
    // ---------------------------------------------------------------------

    wxStaticText* title =
        new wxStaticText(
            root,
            wxID_ANY,
            Config::kAppTitle,
            wxDefaultPosition,
            wxDefaultSize,
            wxALIGN_CENTRE_HORIZONTAL
        );

    title->SetForegroundColour(
        Theme::TextPrimary
    );

    title->SetFont(
        Theme::TitleFont()
    );

    // ---------------------------------------------------------------------
    // Subtitle
    // ---------------------------------------------------------------------

    wxStaticText* subtitle =
        new wxStaticText(
            root,
            wxID_ANY,
            "Sign in to continue",
            wxDefaultPosition,
            wxDefaultSize,
            wxALIGN_CENTRE_HORIZONTAL
        );

    subtitle->SetForegroundColour(
        Theme::TextMuted
    );

    subtitle->SetFont(
        Theme::SubtitleFont()
    );

    rootSizer->Add(
        title,
        0,
        wxALIGN_CENTRE_HORIZONTAL
    );

    rootSizer->Add(
        subtitle,
        0,
        wxALIGN_CENTRE_HORIZONTAL |
        wxBOTTOM,
        24
    );

    // ---------------------------------------------------------------------
    // Username
    // ---------------------------------------------------------------------

    wxStaticText* userLabel =
        new wxStaticText(
            root,
            wxID_ANY,
            "Username"
        );

    userLabel->SetForegroundColour(
        Theme::TextMuted
    );

    userLabel->SetFont(
        Theme::SmallFont().Italic()
    );

    m_usernameCtrl =
        new wxTextCtrl(
            root,
            ID_UsernameCtrl,
            "",
            wxDefaultPosition,
            wxSize(260, 34),
            wxTE_PROCESS_ENTER
        );

    Theme::StyleInput(
        m_usernameCtrl
    );

    // ---------------------------------------------------------------------
    // Password
    // ---------------------------------------------------------------------

    wxStaticText* passLabel =
        new wxStaticText(
            root,
            wxID_ANY,
            "Password"
        );

    passLabel->SetForegroundColour(
        Theme::TextMuted
    );

    passLabel->SetFont(
        Theme::SmallFont().Italic()
    );

    m_passwordCtrl =
        new wxTextCtrl(
            root,
            ID_PasswordCtrl,
            "",
            wxDefaultPosition,
            wxSize(260, 34),
            wxTE_PROCESS_ENTER |
            wxTE_PASSWORD
        );

    Theme::StyleInput(
        m_passwordCtrl
    );

    // ---------------------------------------------------------------------
    // Error label
    // ---------------------------------------------------------------------

    m_errorLabel =
        new wxStaticText(
            root,
            wxID_ANY,
            "",
            wxDefaultPosition,
            wxSize(260, -1),
            wxALIGN_CENTRE_HORIZONTAL
        );

    m_errorLabel->SetForegroundColour(
        Theme::TextDanger
    );

    m_errorLabel->SetFont(
        Theme::SmallFont().Italic()
    );

    m_errorLabel->Hide();

    // ---------------------------------------------------------------------
    // Login button
    // ---------------------------------------------------------------------

    m_loginButton =
        new wxButton(
            root,
            ID_LoginButton,
            "Login",
            wxDefaultPosition,
            wxSize(260, 38)
        );

    Theme::StyleAccentButton(
        m_loginButton
    );

    // ---------------------------------------------------------------------
    // Layout
    // ---------------------------------------------------------------------

    rootSizer->Add(
        userLabel,
        0,
        wxALIGN_CENTRE_HORIZONTAL
    );

    rootSizer->Add(
        m_usernameCtrl,
        0,
        wxALIGN_CENTRE_HORIZONTAL |
        wxBOTTOM,
        12
    );

    rootSizer->Add(
        passLabel,
        0,
        wxALIGN_CENTRE_HORIZONTAL
    );

    rootSizer->Add(
        m_passwordCtrl,
        0,
        wxALIGN_CENTRE_HORIZONTAL |
        wxBOTTOM,
        8
    );

    rootSizer->Add(
        m_errorLabel,
        0,
        wxALIGN_CENTRE_HORIZONTAL |
        wxBOTTOM,
        4
    );

    rootSizer->Add(
        m_loginButton,
        0,
        wxALIGN_CENTRE_HORIZONTAL |
        wxTOP,
        8
    );

    rootSizer->AddStretchSpacer();

    // ---------------------------------------------------------------------
    // Discord advertisement
    // ---------------------------------------------------------------------

    wxStaticText* ad =
        Theme::CreateAdLabel(
            root,
            Config::kDiscordAd
        );

    rootSizer->Add(
        ad,
        0,
        wxALIGN_CENTRE_HORIZONTAL |
        wxBOTTOM,
        12
    );

    root->SetSizer(
        rootSizer
    );

    wxBoxSizer* frameSizer =
        new wxBoxSizer(wxVERTICAL);

    frameSizer->Add(
        root,
        1,
        wxEXPAND
    );

    SetSizer(
        frameSizer
    );

    // ---------------------------------------------------------------------
    // Tab order
    // ---------------------------------------------------------------------

    m_passwordCtrl->MoveAfterInTabOrder(
        m_usernameCtrl
    );

    m_loginButton->MoveAfterInTabOrder(
        m_passwordCtrl
    );

    // ---------------------------------------------------------------------
    // Events
    // ---------------------------------------------------------------------

    m_usernameCtrl->Bind(
        wxEVT_TEXT_ENTER,
        &LoginFrame::OnUsernameEnter,
        this
    );

    m_passwordCtrl->Bind(
        wxEVT_TEXT_ENTER,
        &LoginFrame::OnPasswordEnter,
        this
    );

    m_loginButton->Bind(
        wxEVT_BUTTON,
        &LoginFrame::OnLoginClicked,
        this
    );

    m_usernameCtrl->SetFocus();
}

void LoginFrame::EnsureClientUpToDate()
{
    // Gate the entire tool -- login included -- on the client's own
    // version. This is deliberately a SEPARATE check/version from the
    // module's GitHub release tag (Config::version/Config::assetName):
    // reusing one version string for both would mean bumping one
    // silently affects the other.

    AppUpdater updater(Config::kFirebaseDatabaseUrl);

    const AppUpdater::VersionInfo info = updater.CheckForUpdate();

    if (!info.success)
    {
        // Fail safe: couldn't verify the client is current, so don't
        // unlock login either. The user can close and retry.
        ShowError(
            info.error.empty()
            ? wxString("Update check failed.")
            : wxString::Format("Update check failed: %s", info.error)
        );

        m_loginButton->SetLabel("Login");

        return;
    }

    const bool updateRequired =
        info.forceUpdate ||
        AppUpdater::IsNewerVersion(Config::kAppVersion, info.latestVersion);

    if (!updateRequired)
    {
        SetBusy(false);
        m_loginButton->SetLabel("Login");

        return;
    }

    ShowError("A required update is available. Downloading...");

    wchar_t tempPath[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempPath);

    std::wstring downloadedExePath = tempPath;
    downloadedExePath += L"WxLoginUpdate.exe";

    if (!updater.DownloadFile(info.downloadUrl, downloadedExePath))
    {
        ShowError("Update download failed. Please try again later.");
        m_loginButton->SetLabel("Login");

        return; // Update is required -- form stays locked either way.
    }

    wchar_t currentExePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, currentExePath, MAX_PATH);

    if (!updater.LaunchUpdaterAndExit(downloadedExePath, currentExePath))
    {
        ShowError("Could not launch the updater. Please reinstall the client.");
        m_loginButton->SetLabel("Login");

        return; // Update is still required -- form stays locked.
    }

    Close(true);
}

void LoginFrame::OnUsernameEnter(
    wxCommandEvent&)
{
    m_passwordCtrl->SetFocus();
}

void LoginFrame::OnPasswordEnter(
    wxCommandEvent&)
{
    AttemptLogin();
}

void LoginFrame::OnLoginClicked(
    wxCommandEvent&)
{
    AttemptLogin();
}

void LoginFrame::SetBusy(
    bool busy)
{
    m_usernameCtrl->Enable(!busy);
    m_passwordCtrl->Enable(!busy);
    m_loginButton->Enable(!busy);

    m_loginButton->SetLabel(
        busy
        ? "Signing in..."
        : "Login"
    );
}

void LoginFrame::ShowError(
    const wxString& message)
{
    m_errorLabel->SetLabel(
        message
    );

    m_errorLabel->Show();

    Layout();
}

void LoginFrame::AttemptLogin()
{
    // ---------------------------------------------------------------------
    // Get credentials
    // ---------------------------------------------------------------------

    const wxString username =
        m_usernameCtrl
        ->GetValue()
        .Trim()
        .Trim(false);

    const wxString password =
        m_passwordCtrl->GetValue();

    // ---------------------------------------------------------------------
    // Local validation
    // ---------------------------------------------------------------------

    if (username.IsEmpty())
    {
        ShowError(
            "Please enter your username."
        );

        m_usernameCtrl->SetFocus();
        return;
    }

    if (password.IsEmpty())
    {
        ShowError(
            "Please enter your password."
        );

        m_passwordCtrl->SetFocus();
        return;
    }

    // ---------------------------------------------------------------------
    // Prepare UI
    // ---------------------------------------------------------------------

    m_errorLabel->Hide();

    Layout();

    SetBusy(true);

    // ---------------------------------------------------------------------
    // Authenticate
    // ---------------------------------------------------------------------

    const FirebaseAuth::Result result =
        m_auth->Login(
            username.ToStdString(wxConvUTF8),
            password.ToStdString(wxConvUTF8)
        );

    // ---------------------------------------------------------------------
    // Login attempt finished
    // ---------------------------------------------------------------------

    SetBusy(false);

    if (!result.success)
    {
        /*
            Do not leave the user's password in the control
            after a failed authentication attempt.
        */

        m_passwordCtrl->Clear();

        /*
            FirebaseAuth should already have converted backend
            errors into safe user-facing messages.

            For example:

                Username or Password is wrong.

            instead of:

                INVALID_EMAIL
                EMAIL_NOT_FOUND
                INVALID_PASSWORD
        */

        if (!result.error.empty())
        {
            ShowError(
                wxString::FromUTF8(
                    result.error.c_str()
                )
            );
        }
        else
        {
            ShowError(
                "Authentication failed."
            );
        }

        m_passwordCtrl->SetFocus();

        return;
    }

    // ---------------------------------------------------------------------
    // Successful login
    // ---------------------------------------------------------------------

    const bool isDisabled =
        m_logs->IsAccountDisabled(m_auth->GetEmail(), m_auth->GetIdToken());

    const FirebaseLogs::LicenseResult licenseResult =
        m_logs->CheckLicense(
            m_auth->GetEmail(),
            m_auth->GetIdToken()
        );

    VM_START

        bool blocked = false;
    wxString blockMessage;

    if (isDisabled)
    {
        blocked = true;
        blockMessage = "Your account was disabled\nPlease contact us.";
    }
    else if (licenseResult.status == FirebaseLogs::LicenseStatus::Expired)
    {
        blocked = true;
        blockMessage = "Your license has expired.\nPlease contact us to renew.";
    }

    if (blocked)
    {
        m_auth->Logout();

        m_passwordCtrl->Clear();

        ShowError(blockMessage);

        m_passwordCtrl->SetFocus();

        VM_END
            return;
    }

    if (licenseResult.status == FirebaseLogs::LicenseStatus::Error)
    {
        wxLogDebug(
            "CheckLicense failed: %s",
            wxString::FromUTF8(licenseResult.error.c_str())
        );

        // TODO: decide how a license-check failure (e.g. network error)
        // should be handled -- currently falls through and lets them in.
    }

    const FirebaseLogs::Result logResult =
        m_logs->RecordLogin(
            m_auth->GetEmail(),
            m_auth->GetIdToken(),
            m_logs->GetHardwareId(),
            //m_logs->GetDummyHardwareId(), // That is only for testings and development purposes -- should stay commented unless u want to test.
            m_logs->GetPublicIpAddress()
        );

    if (!logResult.success)
    {
        wxLogDebug(
            "RecordLogin failed: %s",
            wxString::FromUTF8(logResult.error.c_str())
        );

        // TODO: decide how a failed log write should be handled, if at all.
    }

    if (logResult.flaggedSharing)
    {
        wxLogDebug("Login flagged: hardware ID changed for this account.");

        // TODO: react to a flagged account here.
        // e.g. ShowError(...), return early instead of opening ToolFrame,
        // or just let it through and rely on manual review in the console.
    }

    const wxString email =
        wxString::FromUTF8(
            m_auth->GetEmail().c_str()
        );

    ToolFrame* toolFrame =
        new ToolFrame(email);

    toolFrame->Show();

    /*
        LoginFrame is no longer needed after successful authentication.

        Close(true) causes wxWidgets to destroy the frame through
        the normal close-event lifecycle.
    */

    Close(true);
}