#pragma once

#include <wx/wx.h>

// Shared "modern dark" look used by both LoginFrame and ToolFrame,
// so the two windows read as one product.
namespace Theme
{
    inline const wxColour Background{ 18, 18, 24 };
    inline const wxColour Panel{ 26, 26, 34 };
    inline const wxColour Card{ 32, 32, 42 };

    inline const wxColour Accent{ 88, 101, 242 };       // blurple
    inline const wxColour AccentHover{ 114, 125, 245 };
    inline const wxColour AccentPressed{ 71, 82, 196 };

    inline const wxColour TextPrimary{ 235, 235, 240 };
    inline const wxColour TextMuted{ 142, 142, 158 };
    inline const wxColour TextDanger{ 237, 106, 106 };

    inline const wxColour InputBackground{ 40, 40, 52 };
    inline const wxColour InputBorder{ 58, 58, 74 };
    inline const wxColour InputBorderFocused{ 88, 101, 242 };

    // Applies the base dark background/text colours to a frame or panel.
    void ApplyWindowTheme(wxWindow* window);

    // Styles a wxButton as the filled "accent" call-to-action button.
    void StyleAccentButton(wxButton* button);

    // Styles a wxButton as a flat/secondary button (used for things like
    // a refresh icon button next to the process dropdown).
    void StyleSecondaryButton(wxButton* button);

    // Styles a wxTextCtrl / wxComboBox-like input to match the dark theme.
    void StyleInput(wxWindow* input);

    wxFont TitleFont();
    wxFont SubtitleFont();
    wxFont BodyFont();
    wxFont SmallFont();

    // Small, unobtrusive ".petwussy - Discord" style advertisement label
    // used on both windows.
    wxStaticText* CreateAdLabel(wxWindow* parent, const wxString& text);
}
