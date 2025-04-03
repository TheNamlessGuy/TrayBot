#pragma once

#include <windows.h>

namespace Window {
  typedef void (*PromptCallback)(char* value);

  namespace Input {
    extern HWND window;
    namespace Element {
      extern HWND input;
    }

    void init(HINSTANCE hInstance);
    void show();
    void hide();
  }

  namespace CreateNew {
    extern HWND window;
    namespace Element {
      extern HWND input;
      extern HWND dropdown;
    }

    void init(HINSTANCE hInstance);
    void show();
    void hide();
    void destroy();

    void onCreateNew();
  }

  namespace Prompt {
    extern HWND window;
    namespace Element {
      extern HWND text;
      extern HWND input;
    }

    void init(HINSTANCE hInstance);
    void show(const char* label, const char* value, PromptCallback callback);
    void hide();

    void onOK();
  }

  namespace List {
    extern HWND window;

    void init(HINSTANCE hInstance);
    void show();
    void hide();

    void populate();
    void setScrollHeight();
  }

  namespace FileSystemSelect {
    char* folder();
    char* file();
  }

  void init(HINSTANCE hInstance);
  void hide(HWND const& window);
  void show(HWND const& window);
  void destroy();
}