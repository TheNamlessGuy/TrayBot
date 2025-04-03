#include <windows.h>
#include <stdio.h>

#include "defines.h"
#include "io.h"
#include "windows.h"
#include "events.h"
#include "tray.h"

void registerClass(HINSTANCE hInstance) {
  WNDCLASS wc;

  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
  wc.hIcon = Tray::icon;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hInstance = hInstance;
  wc.lpfnWndProc = Event::handle;
  wc.lpszClassName = TEXT("TrayBot");
  wc.lpszMenuName = 0;
  wc.style = CS_HREDRAW | CS_VREDRAW;

  RegisterClass(&wc);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreviInstance, LPSTR lpCmdLine, int nCmdShow) {
  TCHAR binary[MAX_PATH];
  GetModuleFileName(NULL, binary, MAX_PATH);

  IO::read();
  Tray::icon = ExtractIconA(hInstance, binary, 0);
  registerClass(hInstance);
  Window::init(hInstance);
  Tray::init();
  RegisterHotKey(Window::Input::window, 1, MOD_ALT | MOD_CONTROL | MOD_SHIFT, 0x51);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}