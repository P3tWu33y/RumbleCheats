#pragma once
#include <wx/wx.h>
#include <vector>

class MainFrame : public wxFrame
{
public:
    MainFrame();

private:
    std::vector<wxCheckBox*> m_featureCheckBoxes;
};