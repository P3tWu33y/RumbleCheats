#include "App.h"
#include "MainFrame.h"

wxIMPLEMENT_APP_NO_MAIN(App);

bool App::OnInit()
{
    OutputDebugStringA("[WX] App::OnInit entered.\n");

    OutputDebugStringA("[WX] Creating MainFrame.\n");

    MainFrame* frame = new MainFrame();

    OutputDebugStringA("[WX] MainFrame created.\n");

    frame->Show(true);

    OutputDebugStringA("[WX] MainFrame shown.\n");

    return true;
}