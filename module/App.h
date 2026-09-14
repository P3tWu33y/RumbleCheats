#pragma once
#include <wx/wx.h>

extern wxString g_username;

class App : public wxApp
{
public:
    bool OnInit() override;
};

