#include "events.h"

#include "defines.h"
#include "windows.h"
#include "tray.h"

namespace Event {
  namespace {
    void destroy() {
      Window::destroy();
      Tray::destroy();
    }

    void handleButtonClick(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
      switch (LOWORD(wparam)) {
        case BTN_CREATE_NEW_OK:
          Window::CreateNew::onCreateNew();
          break;
        case BTN_PROMPT_OK:
          Window::Prompt::onOK();
          break;
      }
    }

    void handleScrolling(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
      SCROLLINFO si;
      si.cbSize = sizeof(si);
      si.fMask = SIF_ALL;
      GetScrollInfo(window, SB_VERT, &si);

      int oldPos = si.nPos;
      switch (LOWORD(wparam)) {
        case SB_LINEUP:
          si.nPos -= 1;
          break;
        case SB_LINEDOWN:
          si.nPos += 1;
          break;
        case SB_PAGEUP:
          si.nPos -= si.nPage;
          break;
        case SB_PAGEDOWN:
          si.nPos += si.nPage;
          break;
        case SB_THUMBTRACK:
          si.nPos = si.nTrackPos;
          break;
      }

      si.fMask = SIF_POS;
      SetScrollInfo(window, SB_VERT, &si, TRUE);

      GetScrollInfo(window, SB_VERT, &si);
      if (si.nPos != oldPos) {
        ScrollWindow(window, 0, CHARSIZE * (oldPos - si.nPos), NULL, NULL);
        UpdateWindow(window);
      }
    }
  }

  LRESULT CALLBACK handle(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
      case WM_CLOSE: // Pressed window "X" or ALT + F4
        Window::hide(window);
        break;
      case WM_DESTROY:
        PostQuitMessage(0);
        break;
      case WM_QUIT: // During shutdown
        destroy();
        break;

      case WM_HOTKEY: // Hotkey pressed
        Window::Input::show();
        break;
      case WM_VSCROLL: // User scrolled
        handleScrolling(window, msg, wparam, lparam);
        break;

      case WM_COMMAND: // Clicked on window button
        handleButtonClick(window, msg, wparam, lparam);
        break;

      case TRAY_CALLBACK_ID:
        Tray::handleEvent(window, msg, wparam, lparam);
        break;

      default:
        return DefWindowProc(window, msg, wparam, lparam);
    }

    return 0;
  }
}