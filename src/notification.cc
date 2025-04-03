#include "notification.h"

#include "tray.h"

namespace Notification {
  void show(const char* title, const char* text) {
    Tray::data.uFlags = NIF_INFO;
    strcpy_s(Tray::data.szInfoTitle, title);
    strcpy_s(Tray::data.szInfo, text);

    Shell_NotifyIcon(NIM_MODIFY, &Tray::data);
  }
}