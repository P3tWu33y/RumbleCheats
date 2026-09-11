#include "MainFrame.h"
#include <wx/statline.h>

namespace
{
    wxString GetWelcomeText()
    {
        // Placeholder for now - will be set to the actual logged-in username later.
        const wxString username = "Username";
        return wxString::Format("Welcome %s", username);
    }
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Wussy's Tool", wxDefaultPosition, wxSize(420, 360))
{
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(wxColour(30, 30, 30));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Header
    wxStaticText* welcomeText = new wxStaticText(panel, wxID_ANY, GetWelcomeText());
    wxFont headerFont = welcomeText->GetFont();
    headerFont.SetPointSize(16);
    headerFont.SetWeight(wxFONTWEIGHT_BOLD);
    welcomeText->SetFont(headerFont);
    welcomeText->SetForegroundColour(*wxWHITE);

    mainSizer->Add(welcomeText, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 20);

    wxStaticLine* separator = new wxStaticLine(panel);
    mainSizer->Add(separator, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    // Feature rows: label on the left, checkbox on the right, aligned in a grid
    wxFlexGridSizer* featureSizer = new wxFlexGridSizer(/*cols*/ 2, /*vgap*/ 12, /*hgap*/ 20);
    featureSizer->AddGrowableCol(0, 1);

    const char* features[] = { "Feature1", "Feature2" };
    for (const char* feature : features)
    {
        wxStaticText* label = new wxStaticText(panel, wxID_ANY, feature);
        label->SetForegroundColour(*wxWHITE);
        wxFont labelFont = label->GetFont();
        labelFont.SetPointSize(11);
        label->SetFont(labelFont);

        wxCheckBox* checkBox = new wxCheckBox(panel, wxID_ANY, wxEmptyString);

        featureSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
        featureSizer->Add(checkBox, 0, wxALIGN_CENTER_VERTICAL);

        m_featureCheckBoxes.push_back(checkBox);
    }

    mainSizer->Add(featureSizer, 0, wxEXPAND | wxALL, 20);

    panel->SetSizer(mainSizer);
    Centre();
}