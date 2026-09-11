#pragma once
#include <wx/wx.h>
#include "ProcessUtils.h"

class ToolFrame : public wxFrame
{
public:
    explicit ToolFrame(const wxString& loggedInAsUser);

private:
    void BuildUi();
    void OnPollTimer(wxTimerEvent& event);

    // Called once RumbleFighter.exe is detected. Fill in your logic here.
    void OnProcessFound(DWORD pid);

    wxString m_loggedInAsUser;
    wxStaticText* m_statusLabel = nullptr;

    wxTimer m_pollTimer;
    int m_dotCount = 0;
    int m_dotDirection = 1;
    bool m_processFound = false;
};