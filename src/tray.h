#pragma once

#include <windows.h>

namespace Tray {
  extern HICON icon;
  extern HMENU popupMenu;
  extern NOTIFYICONDATA data;

  void init();
  void handleEvent(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);
  void destroy();
}