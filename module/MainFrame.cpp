#include "MainFrame.h"
#include "scanner.h"
#include "memory.h"
#include "resolver.h"
#include <wx/statline.h>




namespace
{
    wxString GetWelcomeText()
    {
        // Placeholder for now - will be set to the actual logged-in username later.
        //const wxString username = "Username";
        //return wxString::Format("Welcome %s", username);

        return wxString("Wussy's Tool");
    }
}

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_CHECKBOX(wxID_HIGHEST + 1, MainFrame::OnFeature1)
EVT_CHECKBOX(wxID_HIGHEST + 2, MainFrame::OnFeature2)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Wussy's Tool",
        wxDefaultPosition, wxSize(420, 360))
{
    wxPanel* panel = new wxPanel(this);
    panel->SetBackgroundColour(wxColour(30, 30, 30));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Header
    wxStaticText* welcomeText =
        new wxStaticText(panel, wxID_ANY, GetWelcomeText());

    wxFont headerFont = welcomeText->GetFont();
    headerFont.SetPointSize(16);
    headerFont.SetWeight(wxFONTWEIGHT_BOLD);

    welcomeText->SetFont(headerFont);
    welcomeText->SetForegroundColour(*wxWHITE);

    mainSizer->Add(
        welcomeText,
        0,
        wxALIGN_CENTER | wxTOP | wxBOTTOM,
        20
    );

    wxStaticLine* separator = new wxStaticLine(panel);

    mainSizer->Add(
        separator,
        0,
        wxEXPAND | wxLEFT | wxRIGHT,
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
    const bool enabled = event.IsChecked();

    Feature1(enabled);
}


// =============================================================
// Feature 2 checkbox
// =============================================================

void MainFrame::OnFeature2(wxCommandEvent& event)
{
    const bool enabled = event.IsChecked();

    Feature2(enabled);
}


// =============================================================
// Feature 1 actual logic
// =============================================================

void MainFrame::Feature1(bool enabled)
{
    if (enabled)
    {
        // -----------------------------------------------------
        // FEATURE 1 ENABLED
        //
        // Put the actual Feature 1 logic here.
        // -----------------------------------------------------

        //wxLogMessage("Feature1 enabled");




        memapi::write(KillAll, "EB"); // This will make massive amount of hits. -- Disabled for public release.


        WriteJmp(KillAll + 0x61, Hit);
        WriteJmp(KillAll + 0x73, Hit);

        WriteJmp(BossKO, KillBossFunc);
        WriteJmp(MonstersKO, KillMonsterFunc);

        // Example:
        //
        // DoSomething();
        //
        // EnableFeature1();
    }
    else
    {
        // -----------------------------------------------------
        // FEATURE 1 DISABLED
        //
        // Put the cleanup / disable logic here.
        // -----------------------------------------------------

        //wxLogMessage("Feature1 disabled");

        memapi::write(KillAll, "75"); // This will make massive amount of hits. -- Disabled for public release.

        WriteJe(KillAll + 0x61, Hit+0x1C);
        WriteJe(KillAll + 0x73, Hit+0x2E);

        WriteJng(BossKO, KillBossFunc);
        WriteJng(MonstersKO, KillMonsterFunc);

        // Example:
        //
        // DisableFeature1();
    }
}


// =============================================================
// Feature 2 actual logic
// =============================================================

uintptr_t returnAddress = 0;

__declspec(naked) void SetESI5()
{
    __asm
    {
        mov esi, 0x4
        test esi, esi
        jmp returnAddress
    }
}

void MainFrame::Feature2(bool enabled)
{
    if (enabled)
    {
        // -----------------------------------------------------
        // FEATURE 2 ENABLED
        //
        // Put the actual Feature 2 logic here.
        // -----------------------------------------------------

        //wxLogMessage("Feature2 enabled");

        //memapi::write(ChestHack, "8B 75 A4 85 F6");

        uintptr_t OriginalChestHack = ChestHack;
		returnAddress = ChestHack += 0x5;

        WriteJmp(OriginalChestHack, (uintptr_t)&SetESI5);

        // Example:
        //
        // DoSomethingElse();
        //
        // EnableFeature2();
    }
    else
    {
        // -----------------------------------------------------
        // FEATURE 2 DISABLED
        //
        // Put the cleanup / disable logic here.
        // -----------------------------------------------------

        //wxLogMessage("Feature2 disabled");
        memapi::write(ChestHack, "8B 75 A4 85 F6");

        // Example:
        //
        // DisableFeature2();
    }
}

