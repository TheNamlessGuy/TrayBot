#include <Windows.h>
#include <shellapi.h>
#include <stdexcept>

#define NAM_HOTKEY_ID 1
#define NAM_TRAYICON_ID 0
#define NAM_TRAYICON_CBID (WM_USER + 1)

#define NAM_TRAY_SHOWWINDOW 5000
#define NAM_TRAY_ADDNEW 5001
#define NAM_TRAY_EXIT 5002

#define NAM_EDIT_INPUT 0
#define NAM_BTN_ADDNEW 1

HWND createNewHWND;
HWND inputHWND;

HWND inputHWND_input;
WNDPROC oldINPUTProc;

HICON gIcon;
NOTIFYICONDATA gNid;
HMENU gMenu;
bool gDoubleClick;

void parseInput(std::wstring const& str)
{
	std::string str1(str.begin(), str.end());
	MessageBox(inputHWND, TEXT("YOU INPUTTED SOMETHING!"), NULL, 0);
}

void centerWindows()
{
	RECT rect;
	GetWindowRect(createNewHWND, &rect);
	SetWindowPos(createNewHWND, 0, (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, rect.right, rect.bottom, 0);

	GetWindowRect(inputHWND, &rect);
	SetWindowLong(inputHWND, GWL_STYLE, GetWindowLong(inputHWND, GWL_STYLE) && !WS_BORDER && !WS_SIZEBOX && !WS_DLGFRAME);
	SetWindowPos(inputHWND, 0, (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, rect.right, rect.bottom, 0);
}

void menuItems()
{
	AppendMenu(gMenu, MF_STRING, NAM_TRAY_SHOWWINDOW, TEXT("Show Input"));
	AppendMenu(gMenu, MF_STRING, NAM_TRAY_ADDNEW, TEXT("Add New"));
	AppendMenu(gMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(gMenu, MF_STRING, NAM_TRAY_EXIT, TEXT("Quit"));
}

LRESULT CALLBACK newEdit(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_KILLFOCUS:
		ShowWindow(inputHWND, SW_HIDE);
		break;
	case WM_KEYDOWN:
		switch (wparam) {
		case VK_ESCAPE:
			ShowWindow(inputHWND, SW_HIDE);
			return 0;
		case VK_RETURN:
			TCHAR buffer[1024];
			GetWindowText(inputHWND_input, buffer, 1024);
			parseInput(std::wstring(buffer));
			SetWindowText(inputHWND_input, TEXT(""));
			return 0;
		}
	default:
		return CallWindowProc(oldINPUTProc, hwnd, msg, wparam, lparam);
	}
}

void windowStyles()
{
	// Input
	inputHWND_input = CreateWindow(TEXT("edit"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 10, 10, 300, 50, inputHWND, (HMENU) NAM_EDIT_INPUT, NULL, NULL);
	HFONT f = CreateFont(45, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Arial"));
	SendMessage(inputHWND_input, WM_SETFONT, WPARAM(f), TRUE);
	oldINPUTProc = (WNDPROC) SetWindowLongPtr(inputHWND_input, GWLP_WNDPROC, (LONG_PTR) newEdit);

	// Create New
	CreateWindow(TEXT("button"), TEXT("Add New"), WS_VISIBLE | WS_CHILD, 10, 10, 80, 25, createNewHWND, (HMENU) NAM_BTN_ADDNEW, NULL, NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	// Window created
	case WM_CREATE:
		// Create menu for tray icon
		gMenu = CreatePopupMenu();
		menuItems();

		// Create window buttons
		windowStyles();
		break;
	// Hotkey call
	case WM_HOTKEY:
		SetForegroundWindow(inputHWND);
		ShowWindow(inputHWND, SW_SHOW);
		SetFocus(inputHWND_input);
		break;
	// X or ALT+F4
	case WM_CLOSE:
		ShowWindow(inputHWND, SW_HIDE);
		ShowWindow(createNewHWND, SW_HIDE);
		break;
	// Window is being destroyed
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	// Application (in general) should quit
	case WM_QUIT:
		DestroyWindow(inputHWND);
		DestroyWindow(createNewHWND);
		break;
	// Window button click
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case NAM_BTN_ADDNEW:

			break;
		}
		break;
	// Any call from the tray icon
	case NAM_TRAYICON_CBID:
		switch (lparam) {
		case WM_LBUTTONDBLCLK:
			gDoubleClick = true;
			break;
		case WM_LBUTTONUP:
			if (gDoubleClick) {
				ShowWindow(inputHWND, SW_SHOW);
				gDoubleClick = false;
			}
			break;
		case WM_RBUTTONDOWN:
			POINT curPoint;
			GetCursorPos(&curPoint);

			SetForegroundWindow(hwnd);
			UINT clicked = TrackPopupMenu(gMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, NULL);

			switch (clicked) {
			case NAM_TRAY_SHOWWINDOW:
				ShowWindow(inputHWND, SW_SHOW);
				break;
			case NAM_TRAY_ADDNEW:
				ShowWindow(createNewHWND, SW_SHOW);
				break;
			case NAM_TRAY_EXIT:
				PostQuitMessage(0);
				break;
			}
			break;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreviInstance, LPSTR lpCmdLine, int nCmdShow)
{
	gDoubleClick = false;
	gIcon = (HICON)LoadImage(NULL, TEXT("icon.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

	WNDCLASS wc;

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hIcon = gIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = TEXT("TrayBot");
	wc.lpszMenuName = 0;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClass(&wc);

	// Start window
	inputHWND = CreateWindow(TEXT("TrayBot"), TEXT("TrayBot - Input"), WS_OVERLAPPEDWINDOW, 10, 10, 310, 60, NULL, NULL, hInstance, NULL);
	ShowWindow(inputHWND, SW_HIDE);

	createNewHWND = CreateWindow(TEXT("TrayBot"), TEXT("TrayBot - New Input Value"), WS_OVERLAPPEDWINDOW, 10, 10, 800, 600, NULL, NULL, hInstance, NULL);
	ShowWindow(createNewHWND, SW_HIDE);

	// Center the windows to the screen
	centerWindows();

	// Hotkey
	RegisterHotKey(inputHWND, NAM_HOTKEY_ID, MOD_ALT | MOD_CONTROL | MOD_SHIFT, 0x51);

	// Tray icon
	gNid.cbSize = sizeof(NOTIFYICONDATA);
	gNid.hWnd = inputHWND;
	gNid.uID = NAM_TRAYICON_ID;
	gNid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	gNid.uCallbackMessage = NAM_TRAYICON_CBID;
	gNid.hIcon = gIcon;
	wcscpy_s(gNid.szTip, TEXT("TrayBot"));

	Shell_NotifyIcon(NIM_ADD, &gNid);

	// Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &gNid);

	return msg.wParam;
}