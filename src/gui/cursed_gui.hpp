#pragma once

#include <windows.h>

namespace govno::gui {

// Launches the maximally hideous & glorious Win32 GUI window and message loop
int run_cursed_gui_app(HINSTANCE hInstance, int nCmdShow);

} // namespace govno::gui
