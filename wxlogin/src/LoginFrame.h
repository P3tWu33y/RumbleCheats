#pragma once

#include <wx/wx.h>
#include <memory>

#include "firebase/FirebaseAuth.h"
#include "firebase/FirebaseLogs.h"

// The login window shown at app startup. On a successful login it opens
// ToolFrame and closes itself.
class LoginFrame : public wxFrame
{
public:
    LoginFrame();

private:
    void BuildUi();
    void AttemptLogin();
    void SetBusy(bool busy);
    void ShowError(const wxString& message);

    void OnUsernameEnter(wxCommandEvent& event);
    void OnPasswordEnter(wxCommandEvent& event);
    void OnLoginClicked(wxCommandEvent& event);

    wxTextCtrl* m_usernameCtrl = nullptr;
    wxTextCtrl* m_passwordCtrl = nullptr;
    wxButton* m_loginButton = nullptr;
    wxStaticText* m_errorLabel = nullptr;

    std::unique_ptr<FirebaseAuth> m_auth;
    std::unique_ptr<FirebaseLogs> m_logs;
};
