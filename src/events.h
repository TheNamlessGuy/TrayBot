#pragma once

#include <windows.h>

namespace Event {
  LRESULT CALLBACK handle(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
}