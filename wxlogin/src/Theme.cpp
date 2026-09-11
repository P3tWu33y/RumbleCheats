#include "Theme.h"

namespace Theme
{
    void ApplyWindowTheme(wxWindow* window)
    {
        if (!window)
            return;

        window->SetBackgroundColour(Background);
        window->SetForegroundColour(TextPrimary);
    }

    void StyleAccentButton(wxButton* button)
    {
        if (!button)
            return;

        button->SetBackgroundColour(Accent);
        button->SetForegroundColour(*wxWHITE);
        button->SetFont(BodyFont().Bold());

        button->Bind(wxEVT_ENTER_WINDOW, [button](wxMouseEvent& evt)
        {
            button->SetBackgroundColour(AccentHover);
            button->Refresh();
            evt.Skip();
        });

        button->Bind(wxEVT_LEAVE_WINDOW, [button](wxMouseEvent& evt)
        {
            button->SetBackgroundColour(Accent);
            button->Refresh();
            evt.Skip();
        });

        button->Bind(wxEVT_LEFT_DOWN, [button](wxMouseEvent& evt)
        {
            button->SetBackgroundColour(AccentPressed);
            button->Refresh();
            evt.Skip();
        });
    }

    void StyleSecondaryButton(wxButton* button)
    {
        if (!button)
            return;

        button->SetBackgroundColour(Card);
        button->SetForegroundColour(TextPrimary);
        button->SetFont(BodyFont());
    }

    void StyleInput(wxWindow* input)
    {
        if (!input)
            return;

        input->SetBackgroundColour(InputBackground);
        input->SetForegroundColour(TextPrimary);
        input->SetFont(BodyFont());
    }

    wxFont TitleFont()
    {
        return wxFont(20, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    }

    wxFont SubtitleFont()
    {
        return wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    }

    wxFont BodyFont()
    {
        return wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    }

    wxFont SmallFont()
    {
        return wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL);
    }

    wxStaticText* CreateAdLabel(wxWindow* parent, const wxString& text)
    {
        wxStaticText* label = new wxStaticText(parent, wxID_ANY, text,
                                                wxDefaultPosition, wxDefaultSize,
                                                wxALIGN_CENTRE_HORIZONTAL);
        label->SetForegroundColour(TextMuted);
        label->SetFont(SmallFont());
        return label;
    }
}
