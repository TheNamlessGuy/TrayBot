#include "windows.h"

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>

#include "defines.h"
#include "actions.h"
#include "notification.h"
#include "io.h"

namespace Window {
  namespace {
    HFONT inputFont;

    void center() {
      RECT rect;
      int screenX = GetSystemMetrics(SM_CXSCREEN);
      int screenY = GetSystemMetrics(SM_CYSCREEN);

      GetWindowRect(Window::Input::window, &rect);
      SetWindowLong(Window::Input::window, GWL_STYLE, GetWindowLong(Window::Input::window, GWL_STYLE) && !WS_BORDER && !WS_SIZEBOX && !WS_DLGFRAME);
      SetWindowPos(Window::Input::window, 0, (screenX - rect.right) / 2, (screenY - rect.bottom) / 2, rect.right, rect.bottom, 0);

      GetWindowRect(Window::CreateNew::window, &rect);
      SetWindowPos(Window::CreateNew::window, 0, (screenX - rect.right) / 2, (screenY - rect.bottom) / 2, rect.right, rect.bottom, 0);

      GetWindowRect(Window::Prompt::window, &rect);
      SetWindowPos(Window::Prompt::window, 0, (screenX - rect.right) / 2, (screenY - rect.bottom) / 2, rect.right, rect.bottom, 0);

      GetWindowRect(Window::List::window, &rect);
      SetWindowPos(Window::List::window, 0, (screenX - rect.right) / 2, (screenY - rect.bottom) / 2, rect.right, rect.bottom, 0);
    }

    char* getInput(HWND input, int size) {
      char* value = (char*) malloc(size * sizeof(char));
      GetWindowText(input, value, size);
      return value;
    }

    void clearInput(HWND input) {
      SetWindowText(input, TEXT(""));
    }

    char* getAndClearInput(HWND input, int size) {
      char* value = getInput(input, size);
      clearInput(input);
      return value;
    }
  }

  namespace Input {
    namespace {
      void execute(Action::Action* action) {
        if (action->type == ACTION_OPEN_FOLDER) {
          ShellExecute(NULL, TEXT("explore"), action->value, NULL, NULL, SW_SHOW);
        } else if (action->type == ACTION_RUN_PROGRAM) {
          ShellExecute(NULL, NULL, action->value, NULL, NULL, SW_SHOW);
        } else if (action->type == ACTION_SET_CLIPBOARD) {
          long len = strlen(action->value) + 1; // +1 = '\0'
          long wlen = len * sizeof(WCHAR);

          WCHAR* value = (WCHAR*) malloc(wlen);
          MultiByteToWideChar(CP_UTF8, 0, action->value, len, value, wlen);

          HGLOBAL clipboard = GlobalAlloc(GMEM_MOVEABLE, wlen);
          LPVOID lock = GlobalLock(clipboard);
          memcpy(lock, value, wlen);
          GlobalUnlock(clipboard);

          OpenClipboard(NULL);
          EmptyClipboard();
          SetClipboardData(CF_UNICODETEXT, clipboard);
          CloseClipboard();

          free(value);

          long length = 8 + strlen(action->value) + 14 + 1; // 8 = strlen("Copied '"), 14 = strlen("' to clipboard"), 1 = '\0'
          char msg[length];
          snprintf(msg, length, "Copied '%s' to clipboard", action->value);
          Notification::show("Success", msg);
        }
      }

      void onInput(const char* input) {
        if (strlen(input) == 0) { return; }

        for (long i = 0; i < Action::amount; ++i) {
          if (strcmp(Action::list[i]->name, input) == 0) {
            execute(Action::list[i]);
            return;
          }
        }

        long length = 9 + strlen(input) + 11 + 1; // 9 = strlen("Command '"), 11 = strlen("' not found"), 1 = '\0'
        char msg[length];
        snprintf(msg, length, "Command '%s' not found", input);
        Notification::show("Error", msg);
      }

      WNDPROC baseInputHandler;
      LRESULT CALLBACK inputHandler(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
          case WM_KILLFOCUS:
            Window::Input::hide();
            clearInput(Window::Input::Element::input);
            break;
          case WM_KEYDOWN:
            switch (wparam) {
              case VK_ESCAPE:
                Window::Input::hide();
                clearInput(Window::Input::Element::input);
                break;
              case VK_RETURN:
                char* value = getAndClearInput(Window::Input::Element::input, 1024);
                Window::Input::hide();
                onInput(value);
                break;
            }
            break;
          default:
            return CallWindowProc(baseInputHandler, window, msg, wparam, lparam);
        }

        return 0;
      }
    }

    HWND window;
    namespace Element {
      HWND input;
    }

    void init(HINSTANCE hInstance) {
      Window::Input::window = CreateWindow(TEXT("TrayBot"), TEXT("TrayBot - Input"), NULL, 10, 10, 310, 60, NULL, NULL, hInstance, NULL);

      Window::Input::Element::input = CreateWindow(TEXT("edit"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 10, 300, 50, Window::Input::window, (HMENU) 0, NULL, NULL);
      SendMessage(Window::Input::Element::input, WM_SETFONT, WPARAM(inputFont), TRUE);
      baseInputHandler = (WNDPROC) SetWindowLongPtr(Window::Input::Element::input, GWLP_WNDPROC, (LONG_PTR) inputHandler);

      Window::Input::hide();
    }

    void show() {
      Window::show(Window::Input::window);
      SetFocus(Window::Input::Element::input);
    }

    void hide() {
      Window::hide(Window::Input::window);
    }

    void destroy() {
      DestroyWindow(Window::Input::window);
    }
  }

  namespace CreateNew {
    namespace {
      void onCreateClipboardAction(char* value) {
        Action::add(getAndClearInput(Window::CreateNew::Element::input, 1024), ACTION_SET_CLIPBOARD, value);
      }
    }

    HWND window;
    namespace Element {
      HWND input;
      HWND dropdown;
    }

    void init(HINSTANCE hInstance) {
      Window::CreateNew::window = CreateWindow(TEXT("TrayBot"), TEXT("New input value - TrayBot"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 10, 10, 330, 180, NULL, NULL, hInstance, NULL);

      CreateWindow(TEXT("STATIC"), TEXT("New input:"), WS_VISIBLE | WS_CHILD, 10, 10, 100, 20, Window::CreateNew::window, NULL, NULL, NULL);

      Window::CreateNew::Element::input = CreateWindow(TEXT("edit"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP, 10, 30, 300, 50, Window::CreateNew::window, (HMENU) 2, NULL, NULL);
      SendMessage(Window::CreateNew::Element::input, WM_SETFONT, WPARAM(inputFont), TRUE);

      Window::CreateNew::Element::dropdown = CreateWindow(TEXT("COMBOBOX"), NULL, WS_VISIBLE | WS_CHILD | WS_OVERLAPPED | WS_TABSTOP | CBS_DROPDOWNLIST, 10, 90, 300, 100, Window::CreateNew::window, NULL, NULL, NULL);
      SendMessage(Window::CreateNew::Element::dropdown, CB_ADDSTRING, 0, (LPARAM) TEXT("Open folder"));
      SendMessage(Window::CreateNew::Element::dropdown, CB_ADDSTRING, 0, (LPARAM) TEXT("Set clipboard"));
      SendMessage(Window::CreateNew::Element::dropdown, CB_ADDSTRING, 0, (LPARAM) TEXT("Run/Open file"));
      SendMessage(Window::CreateNew::Element::dropdown, CB_SETCURSEL, 0, 0); // Default to index 0

      CreateWindow(TEXT("button"), TEXT("Add new"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 10, 120, 300, 25, Window::CreateNew::window, (HMENU) BTN_CREATE_NEW_OK, NULL, NULL);

      Window::CreateNew::hide();
    }

    void show() {
      Window::show(Window::CreateNew::window);
      SetFocus(Window::CreateNew::Element::input);
    }

    void hide() {
      Window::hide(Window::CreateNew::window);
    }

    void destroy() {
      DestroyWindow(Window::CreateNew::window);
    }

    void onCreateNew() {
      Window::CreateNew::hide();
      int idx = SendMessage(Window::CreateNew::Element::dropdown, CB_GETCURSEL, 0, 0);
      switch (idx) {
        case ACTION_OPEN_FOLDER: {
            char* value = Window::FileSystemSelect::folder();
            char* name = getAndClearInput(Window::CreateNew::Element::input, 1024);
            Action::add(name, ACTION_OPEN_FOLDER, value);
            break;
          }
        case ACTION_SET_CLIPBOARD:
          Window::Prompt::show("Set clipboard to:", "", onCreateClipboardAction);
          break;
        case ACTION_RUN_PROGRAM: {
            char* value = Window::FileSystemSelect::file();
            char* name = getAndClearInput(Window::CreateNew::Element::input, 1024);
            Action::add(name, ACTION_RUN_PROGRAM, value);
            break;
          }
      }
    }
  }

  namespace Prompt {
    namespace {
      PromptCallback cb;
    }

    HWND window;
    namespace Element {
      HWND text;
      HWND input;
    }

    void init(HINSTANCE hInstance) {
      Window::Prompt::window = CreateWindow(TEXT("TrayBot"), TEXT("Prompt - TrayBot"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 10, 10, 330, 150, NULL, NULL, hInstance, NULL);

      Window::Prompt::Element::text = CreateWindow(TEXT("STATIC"), TEXT(""), WS_VISIBLE | WS_CHILD, 10, 10, 300, 20, Window::Prompt::window, NULL, NULL, NULL);
      Window::Prompt::Element::input = CreateWindow(TEXT("edit"), NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP, 10, 30, 300, 50, Window::Prompt::window, (HMENU) 3, NULL, NULL);
      SendMessage(Window::Prompt::Element::input, WM_SETFONT, WPARAM(inputFont), TRUE);
      CreateWindow(TEXT("button"), TEXT("OK"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 10, 90, 300, 25, Window::Prompt::window, (HMENU) BTN_PROMPT_OK, NULL, NULL);

      Window::Prompt::hide();
    }

    void show(const char* label, const char* value, PromptCallback callback) {
      cb = callback;
      SetWindowText(Window::Prompt::Element::text, label);
      SetWindowText(Window::Prompt::Element::input, value);
      Window::show(Window::Prompt::window);
    }

    void hide() {
      Window::hide(Window::Prompt::window);
    }

    void destroy() {
      DestroyWindow(Window::Prompt::window);
    }

    void onOK() {
      Window::Prompt::hide();
      cb(getAndClearInput(Window::Prompt::Element::input, 1024));
    }
  }

  namespace List {
    HWND window;

    void init(HINSTANCE hInstance) {
      Window::List::window = CreateWindow(TEXT("TrayBot"), TEXT("List - TrayBot"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VSCROLL, 10, 10, 330, 250, NULL, NULL, hInstance, NULL);
      Window::List::populate();
      Window::List::hide();
    }

    void show() {
      Window::show(Window::List::window);
    }

    void hide() {
      Window::hide(Window::List::window);
    }

    void destroy() {
      DestroyWindow(Window::List::window);
    }

    namespace {
      int nextListY = 0;

      BOOL CALLBACK DestroyChildrenCallback(HWND hwnd, LPARAM lparam) { // https://stackoverflow.com/a/30787899
        if (hwnd != NULL) {
          DestroyWindow(hwnd);
        }

        return TRUE;
      }
    }

    void populate() {
      EnumChildWindows(Window::List::window, DestroyChildrenCallback, NULL);

      nextListY = 0;
      for (int i = 0; i < Action::amount; ++i) {
        CreateWindow(TEXT("STATIC"), Action::list[i]->name, WS_VISIBLE | WS_CHILD, 10, nextListY, 100, 20, Window::List::window, NULL, NULL, NULL);
        nextListY += 20;
      }

      Window::List::setScrollHeight();
    }

    void setScrollHeight() {
      SCROLLINFO si;
      si.cbSize = sizeof(si);
      si.fMask = SIF_RANGE | SIF_PAGE;
      si.nMin = 0;
      si.nPage = 250 / CHARSIZE;
      si.nMax = Action::amount - 3;
      SetScrollInfo(Window::List::window, SB_VERT, &si, TRUE);
    }
  }

  namespace FileSystemSelect {
    namespace {
      char* open(const char* title, UINT flags) {
        BROWSEINFO bi = {0};
        bi.lpszTitle = title;
        bi.ulFlags = flags;
        bi.lpfn = NULL;

        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl == 0) { return NULL; }

        char* pathBuffer = (char*) malloc(MAX_PATH * sizeof(char));
        SHGetPathFromIDList(pidl, pathBuffer);

        IMalloc* imalloc;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
          imalloc->Free(pidl);
          imalloc->Release();
        }

        return pathBuffer;
      }
    }

    char* folder() {
      return open("Select a folder", BIF_RETURNONLYFSDIRS);
    }

    char* file() {
      return open("Select a file", BIF_BROWSEINCLUDEFILES);
    }
  }

  void init(HINSTANCE hInstance) {
    inputFont = CreateFontA(45, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Arial"));

    Window::Input::init(hInstance);
    Window::CreateNew::init(hInstance);
    Window::Prompt::init(hInstance);
    Window::List::init(hInstance);

    center();
  }

  void show(HWND const& window) {
    SetForegroundWindow(window);
    ShowWindow(window, SW_SHOW);
  }

  void hide(HWND const& window) {
    ShowWindow(window, SW_HIDE);
  }

  void destroy() {
    Window::List::destroy();
    Window::Prompt::destroy();
    Window::CreateNew::destroy();
    Window::Input::destroy();

    DeleteObject(inputFont);
  }
}