#include "tray.h"

#include "windows.h"
#include "defines.h"

namespace Tray {
  namespace {
    bool doubleClick = false;
  }

  HICON icon;
  HMENU popupMenu;
  NOTIFYICONDATA data;

  void init() {
    Tray::popupMenu = CreatePopupMenu();
    AppendMenu(Tray::popupMenu, MF_STRING, TRAY_SHOW_WINDOW, TEXT("Show input"));
    AppendMenu(Tray::popupMenu, MF_STRING, TRAY_ADD_NEW, TEXT("Add new"));
    AppendMenu(Tray::popupMenu, MF_STRING, TRAY_LIST_ALL, TEXT("List all"));
    AppendMenu(Tray::popupMenu, MF_STRING, TRAY_REMOVE, TEXT("Remove"));
    AppendMenu(Tray::popupMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(Tray::popupMenu, MF_STRING, TRAY_EXIT, TEXT("Quit"));

    Tray::data.cbSize = sizeof(NOTIFYICONDATA);
    Tray::data.hWnd = Window::Input::window;
    Tray::data.uID = 0;
    Tray::data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    Tray::data.uCallbackMessage = TRAY_CALLBACK_ID;
    Tray::data.uVersion = NOTIFYICON_VERSION;
    Tray::data.hIcon = Tray::icon;
    strcpy_s(Tray::data.szTip, TEXT("TrayBot"));

    Shell_NotifyIcon(NIM_ADD, &Tray::data);
  }

  void handleEvent(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (lparam) {
      case WM_LBUTTONDBLCLK:
        doubleClick = true;
        break;
      case WM_LBUTTONUP:
        if (doubleClick) {
          Window::Input::show();
          doubleClick = false;
        }
        break;
      case WM_RBUTTONDOWN:
        SetFocus(window);
        SetForegroundWindow(window);

        POINT cursor;
        GetCursorPos(&cursor);
        UINT clicked = TrackPopupMenu(Tray::popupMenu, TPM_RETURNCMD | TPM_NONOTIFY, cursor.x, cursor.y, 0, window, NULL);
        switch (clicked) {
          case TRAY_SHOW_WINDOW:
            Window::Input::show();
            break;
          case TRAY_ADD_NEW:
            Window::CreateNew::show();
            break;
          case TRAY_LIST_ALL:
            Window::List::show();
            break;
          case TRAY_REMOVE:
            // TODO
            break;
          case TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }

        break;
    }
  }

  void destroy() {
    Shell_NotifyIcon(NIM_DELETE, &Tray::data);
  }
}