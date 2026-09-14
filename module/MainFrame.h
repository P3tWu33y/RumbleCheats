#pragma once

#include <wx/wx.h>
#include <vector>

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& username);

private:
    void OnFeature1(wxCommandEvent& event);
    void OnFeature2(wxCommandEvent& event);

    void Feature1(bool enabled);
    void Feature2(bool enabled);

    std::vector<wxCheckBox*> m_featureCheckBoxes;

    wxDECLARE_EVENT_TABLE();
};