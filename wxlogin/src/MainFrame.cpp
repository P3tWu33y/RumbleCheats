#include "MainFrame.h"
#include <wx/statline.h>
#include <thread>
#include "SharedParams.h"
#include "IPCClient.h"
#include "IPC.h"

void MainFrame::OnClose(wxCloseEvent&)
{
    wxTheApp->ExitMainLoop();
}

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_CHECKBOX(wxID_HIGHEST + 1, MainFrame::OnFeature1)
EVT_CHECKBOX(wxID_HIGHEST + 2, MainFrame::OnFeature2)
EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& username)
    : wxFrame(nullptr, wxID_ANY, "Wussy's Tool",
        wxDefaultPosition, wxSize(420, 360))
{
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(wxColour(30, 30, 30));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Header
    wxFont headerFont;
    headerFont.SetPointSize(16);
    headerFont.SetWeight(wxFONTWEIGHT_BOLD);

    wxBoxSizer* welcomeSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* welcomeLabel =
        new wxStaticText(panel, wxID_ANY, "Welcome");

    welcomeLabel->SetFont(headerFont);
    welcomeLabel->SetForegroundColour(*wxWHITE);

    wxStaticText* usernameLabel =
        new wxStaticText(panel, wxID_ANY, username);

    usernameLabel->SetFont(headerFont);
    usernameLabel->SetForegroundColour(wxColour(100, 180, 255));

    welcomeSizer->Add(
        welcomeLabel,
        0,
        wxALIGN_CENTER | wxBOTTOM,
        2
    );

    welcomeSizer->Add(
        usernameLabel,
        0,
        wxALIGN_CENTER
    );

    mainSizer->Add(
        welcomeSizer,
        0,
        wxALIGN_CENTER | wxTOP | wxBOTTOM,
        10
    );

    // Separator
    wxStaticLine* separator = new wxStaticLine(panel);

    mainSizer->Add(
        separator,
        0,
        wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM,
        20
    );

    // Feature rows
    wxFlexGridSizer* featureSizer =
        new wxFlexGridSizer(2, 12, 20);

    featureSizer->AddGrowableCol(0, 1);

    // ---------------------------------------------------------
    // Feature 1
    // ---------------------------------------------------------

    wxStaticText* feature1Label =
        new wxStaticText(panel, wxID_ANY, "Kill-All");

    feature1Label->SetForegroundColour(*wxWHITE);

    wxFont feature1Font = feature1Label->GetFont();
    feature1Font.SetPointSize(11);
    feature1Label->SetFont(feature1Font);

    wxCheckBox* feature1CheckBox =
        new wxCheckBox(
            panel,
            wxID_HIGHEST + 1,
            wxEmptyString
        );

    featureSizer->Add(
        feature1Label,
        0,
        wxALIGN_CENTER_VERTICAL
    );

    featureSizer->Add(
        feature1CheckBox,
        0,
        wxALIGN_CENTER_VERTICAL
    );

    m_featureCheckBoxes.push_back(feature1CheckBox);

    // ---------------------------------------------------------
    // Feature 2
    // ---------------------------------------------------------

    wxStaticText* feature2Label =
        new wxStaticText(panel, wxID_ANY, "ChestHack x4");

    feature2Label->SetForegroundColour(*wxWHITE);

    wxFont feature2Font = feature2Label->GetFont();
    feature2Font.SetPointSize(11);
    feature2Label->SetFont(feature2Font);

    wxCheckBox* feature2CheckBox =
        new wxCheckBox(
            panel,
            wxID_HIGHEST + 2,
            wxEmptyString
        );

    featureSizer->Add(
        feature2Label,
        0,
        wxALIGN_CENTER_VERTICAL
    );

    featureSizer->Add(
        feature2CheckBox,
        0,
        wxALIGN_CENTER_VERTICAL
    );

    m_featureCheckBoxes.push_back(feature2CheckBox);

    mainSizer->Add(
        featureSizer,
        0,
        wxEXPAND | wxALL,
        20
    );

    panel->SetSizer(mainSizer);

    Centre();
}


// =============================================================
// Feature 1 checkbox
// =============================================================

void MainFrame::OnFeature1(wxCommandEvent& event)
{
    bool enabled = event.IsChecked();

    std::thread([enabled]()
        {
            SetFeature(IPCCommand::Feature1, enabled);
        }).detach();
}


// =============================================================
// Feature 2 checkbox
// =============================================================

void MainFrame::OnFeature2(wxCommandEvent& event)
{
    bool enabled = event.IsChecked();

    std::thread([enabled]()
        {
            SetFeature(IPCCommand::Feature2, enabled);
        }).detach();
}