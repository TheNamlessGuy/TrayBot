#include <Windows.h>
#include <shellapi.h>
#include <CommCtrl.h>
#include <Shlobj.h>

#include <string>
#include <tuple>
#include <vector>
#include <fstream>
#include <sstream>

#include <codecvt>

#define NAM_HOTKEY_ID 1
#define NAM_TRAYICON_ID 0
#define NAM_TRAYICON_CBID (WM_USER + 1)

#define NAM_TRAY_SHOWWINDOW 5000
#define NAM_TRAY_ADDNEW 5001
#define NAM_TRAY_LISTALL 5002
#define NAM_TRAY_EXIT 5003

#define NAM_EDIT_INPUT 0
#define NAM_BTN_ADDNEW 1
#define NAM_EDIT_CREATE 2
#define NAM_EDIT_PROMPT 3
#define NAM_BTN_PROMPT 4

#define NAM_DEFAULTCHARSIZE 25
#define NAM_LIST_HEIGHT 250

enum class ACTION
{
	OPENFOLDER = 6000,
	SETCLIPBOARD = 6001
};

//                           NAME          ACTION  INPUT
#define NAM_ENTRY std::tuple<std::wstring, ACTION, std::wstring>

HWND createNewHWND;
HWND inputHWND;
HWND promptHWND;
HWND listHWND;

HWND promptHWND_input;
HWND promptHWND_text;

HWND createNewHWND_input;
HWND createNewHWND_dropdown;

HWND inputHWND_input;
WNDPROC oldINPUTProc;

HICON gIcon;
NOTIFYICONDATA gNid;
HMENU gTrayMenu;
HMENU gDropDownMenu;
bool gDoubleClick;
ACTION currentPromptAction;
int gListNextY;

std::vector<NAM_ENTRY> gEntries;

void _SHOW(HWND const& hwnd)
{
	SetForegroundWindow(hwnd);
	ShowWindow(hwnd, SW_SHOW);
}

void _HIDE(HWND const& hwnd)
{
	ShowWindow(hwnd, SW_HIDE);
}

void showNotification(std::wstring const& title, std::wstring const& msg)
{
	gNid.uFlags = NIF_INFO;
	wcscpy_s(gNid.szInfoTitle, title.c_str());
	wcscpy_s(gNid.szInfo, msg.c_str());

	Shell_NotifyIcon(NIM_MODIFY, &gNid);
}

void setScrollHeight()
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nMin = 0;
	si.nPage = NAM_LIST_HEIGHT / NAM_DEFAULTCHARSIZE;
	si.nMax = gEntries.size() - 3;
	SetScrollInfo(listHWND, SB_VERT, &si, TRUE);
}

void setList()
{
	gListNextY = 0;
	for (int i = 0; i < gEntries.size(); i++) {
		CreateWindow(TEXT("STATIC"), std::get<0>(gEntries[i]).c_str(), WS_VISIBLE | WS_CHILD, 10, gListNextY, 100, 20, listHWND, NULL, NULL, NULL);
		gListNextY += 20;
	}
	setScrollHeight();
	//setListHeight();
}

void addNewEntry(std::wstring const& name, ACTION const& action, std::wstring const& input)
{
	if (name == TEXT("")) {
		showNotification(TEXT("Error"), TEXT("Values can not be empty"));
		return;
	}

	if (input == TEXT("")) {
		showNotification(TEXT("Error"), TEXT("Values can not be empty"));
		return;
	}

	std::size_t found = name.find('|');
	if (found != std::string::npos) {
		showNotification(TEXT("Error"), TEXT("Values can not contain the character '|'"));
		return;
	}

	found = input.find('|');
	if (found != std::string::npos) {
		showNotification(TEXT("Error"), TEXT("Values can not contain the character '|'"));
		return;
	}

	for (NAM_ENTRY const& ne : gEntries) {
		if (std::get<0>(ne) == name) {
			showNotification(TEXT("Error"), TEXT("Value '") + name + TEXT("' already exists!"));
			return;
		}
	}

	// Everything is fine
	gEntries.push_back(NAM_ENTRY(name, action, input));

	CreateWindow(TEXT("STATIC"), name.c_str(), WS_VISIBLE | WS_CHILD, 10, gListNextY, 100, 20, listHWND, NULL, NULL, NULL);
	gListNextY += 20;
	setScrollHeight();
	//setListHeight();

	std::wofstream ofs(TEXT("data.txt"), std::ios::app);
	ofs << name << '|' << int(action) << '|' << input << std::endl;
	ofs.close();

	_HIDE(createNewHWND);

	showNotification(TEXT("Success"), TEXT("Added new value '") + name + TEXT("'"));
}

void executeEntry(NAM_ENTRY ne)
{
	switch (std::get<1>(ne)) {
	case ACTION::OPENFOLDER: {
		ShellExecute(NULL, TEXT("explore"), std::get<2>(ne).c_str(), NULL, NULL, SW_SHOW);
	}
		break;
	case ACTION::SETCLIPBOARD: {
		std::wstring value = std::get<2>(ne);
		int len = (value.length() + 1) * sizeof(WCHAR);

		HGLOBAL newClip = GlobalAlloc(GMEM_MOVEABLE, len);
		memcpy(GlobalLock(newClip), value.c_str(), len);
		GlobalUnlock(newClip);

		OpenClipboard(0);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, newClip);
		CloseClipboard();

		GlobalFree(newClip);

		showNotification(TEXT("Success"), TEXT("Copied '") + value + TEXT("' to clipboard"));
	}
		break;
	}
}

void parseInput(std::wstring const& str)
{
	for (NAM_ENTRY const& ne : gEntries) {
		if (std::get<0>(ne) == str) {
			executeEntry(ne);
			return;
		}
	}
	showNotification(TEXT("Error"), TEXT("Command '") + str + TEXT("' not found"));
}

void centerWindows()
{
	RECT rect;
	GetWindowRect(createNewHWND, &rect);
	SetWindowPos(createNewHWND, 0, (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, rect.right, rect.bottom, 0);

	GetWindowRect(inputHWND, &rect);
	SetWindowLong(inputHWND, GWL_STYLE, GetWindowLong(inputHWND, GWL_STYLE) && !WS_BORDER && !WS_SIZEBOX && !WS_DLGFRAME);
	SetWindowPos(inputHWND, 0, (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, rect.right, rect.bottom, 0);

	GetWindowRect(promptHWND, &rect);
	SetWindowPos(promptHWND, 0, (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, rect.right, rect.bottom, 0);

	GetWindowRect(listHWND, &rect);
	SetWindowPos(listHWND, 0, (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2, (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2, rect.right, rect.bottom, 0);
}

void menuItems()
{
	AppendMenu(gTrayMenu, MF_STRING, NAM_TRAY_SHOWWINDOW, TEXT("Show Input"));
	AppendMenu(gTrayMenu, MF_STRING, NAM_TRAY_ADDNEW, TEXT("Add New"));
	AppendMenu(gTrayMenu, MF_STRING, NAM_TRAY_LISTALL, TEXT("List all"));
	AppendMenu(gTrayMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(gTrayMenu, MF_STRING, NAM_TRAY_EXIT, TEXT("Quit"));
}

LRESULT CALLBACK newINPUTEdit(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_KILLFOCUS:
		_HIDE(inputHWND);
		return 0;
	case WM_KEYDOWN:
		switch (wparam) {
		case VK_ESCAPE:
			_HIDE(inputHWND);
			return 0;
		case VK_RETURN:
			TCHAR buffer[1024];
			GetWindowText(inputHWND_input, buffer, 1024);
			parseInput(std::wstring(buffer));
			SetWindowText(inputHWND_input, TEXT(""));
			_HIDE(inputHWND);
			return 0;
		}
	default:
		return CallWindowProc(oldINPUTProc, hwnd, msg, wparam, lparam);
	}
}

void dropdownitems()
{
	SendMessage(createNewHWND_dropdown, CB_ADDSTRING, 0, (LPARAM)TEXT("Open Folder"));
	SendMessage(createNewHWND_dropdown, CB_ADDSTRING, 0, (LPARAM)TEXT("Set Clipboard"));

	// Set index of intially shown item
	SendMessage(createNewHWND_dropdown, CB_SETCURSEL, 0, 0);
}

void windowStyles()
{
	// Input
	inputHWND_input = CreateWindow(TEXT("edit"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 10, 300, 50, inputHWND, (HMENU) NAM_EDIT_INPUT, NULL, NULL);
	HFONT f = CreateFont(45, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Arial"));
	SendMessage(inputHWND_input, WM_SETFONT, WPARAM(f), TRUE);
	oldINPUTProc = (WNDPROC) SetWindowLongPtr(inputHWND_input, GWLP_WNDPROC, (LONG_PTR) newINPUTEdit);

	// Prompt
	promptHWND_text = CreateWindow(TEXT("STATIC"), TEXT(""), WS_VISIBLE | WS_CHILD, 10, 10, 300, 20, promptHWND, NULL, NULL, NULL);
	promptHWND_input = CreateWindow(TEXT("edit"), NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP, 10, 30, 300, 50, promptHWND, (HMENU)NAM_EDIT_PROMPT, NULL, NULL);
	SendMessage(promptHWND_input, WM_SETFONT, WPARAM(f), TRUE);
	CreateWindow(TEXT("button"), TEXT("OK"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 10, 90, 300, 25, promptHWND, (HMENU)NAM_BTN_PROMPT, NULL, NULL);

	// CREATE NEW
	// "New Input" text
	CreateWindow(TEXT("STATIC"), TEXT("New input:"), WS_VISIBLE | WS_CHILD, 10, 10, 100, 20, createNewHWND, NULL, NULL, NULL);

	// Input box
	createNewHWND_input = CreateWindow(TEXT("edit"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP, 10, 30, 300, 50, createNewHWND, (HMENU) NAM_EDIT_CREATE, NULL, NULL);
	SendMessage(createNewHWND_input, WM_SETFONT, WPARAM(f), TRUE);

	// Dropdown menu
	createNewHWND_dropdown = CreateWindow(TEXT("COMBOBOX"), NULL, WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_TABSTOP | CBS_DROPDOWNLIST, 10, 90, 300, 100, createNewHWND, NULL, NULL, NULL);
	dropdownitems();

	// Ok button
	CreateWindow(TEXT("button"), TEXT("Add New"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 10, 120, 300, 25, createNewHWND, (HMENU) NAM_BTN_ADDNEW, NULL, NULL);
}

void showPrompt(std::wstring promptText)
{
	SetWindowText(promptHWND_text, promptText.c_str());
	_SHOW(promptHWND);
}

std::wstring selectFolder()
{
	BROWSEINFO bi = { 0 };
	bi.lpszTitle = TEXT("Select folder to open");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl == 0) { return TEXT(""); }

	TCHAR pathBuffer[MAX_PATH];
	SHGetPathFromIDList(pidl, pathBuffer);

	IMalloc* imalloc;
	if (SUCCEEDED(SHGetMalloc(&imalloc))) {
		imalloc->Free(pidl);
		imalloc->Release();
	}

	return pathBuffer;
}

void handleScrolling(HWND hwnd, WPARAM wparam)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &si);

	int yPos = si.nPos;
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
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
	GetScrollInfo(hwnd, SB_VERT, &si);

	if (si.nPos != yPos) {
		ScrollWindow(hwnd, 0, NAM_DEFAULTCHARSIZE * (yPos - si.nPos), NULL, NULL);
		UpdateWindow(hwnd);
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	// Hotkey call
	case WM_HOTKEY:
		_SHOW(inputHWND);
		SetFocus(inputHWND_input);
		break;
	// X or ALT+F4
	case WM_CLOSE:
		_HIDE(hwnd);
		break;
	// Window is being destroyed
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	// Application (in general) should quit
	case WM_QUIT:
		DestroyWindow(promptHWND);
		DestroyWindow(inputHWND);
		DestroyWindow(createNewHWND);
		break;
	// User scrolled
	case WM_VSCROLL:
		handleScrolling(hwnd, wparam);
		break;
	// Window button click
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		// Create new button
		case NAM_BTN_ADDNEW: {
			int index = SendMessage(createNewHWND_dropdown, CB_GETCURSEL, 0, 0) + 6000;
			switch (index) {
			case int(ACTION::OPENFOLDER) : {
				std::wstring path = selectFolder();

				TCHAR buffer[1024];
				GetWindowText(createNewHWND_input, buffer, 1024);
				SetWindowText(createNewHWND_input, TEXT(""));
				addNewEntry(std::wstring(buffer), ACTION::OPENFOLDER, path);
			}
										   break;
			case int(ACTION::SETCLIPBOARD) :
				currentPromptAction = ACTION::SETCLIPBOARD;
				showPrompt(TEXT("Set clipboard to:"));
				break;
			}
		}
			break;
		// Prompt button
		case NAM_BTN_PROMPT: {
			_HIDE(promptHWND);

			TCHAR prompt[1024];
			GetWindowText(promptHWND_input, prompt, 1024);
			SetWindowText(promptHWND_input, TEXT(""));

			TCHAR value[1024];
			GetWindowText(createNewHWND_input, value, 1024);
			SetWindowText(createNewHWND_input, TEXT(""));

			addNewEntry(std::wstring(value), currentPromptAction, std::wstring(prompt));
		}
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
				_SHOW(inputHWND);
				SetFocus(inputHWND_input);
				gDoubleClick = false;
			}
			break;
		case WM_RBUTTONDOWN:
			SetFocus(hwnd);

			POINT curPoint;
			GetCursorPos(&curPoint);

			SetForegroundWindow(hwnd);
			UINT clicked = TrackPopupMenu(gTrayMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hwnd, NULL);

			switch (clicked) {
			case NAM_TRAY_SHOWWINDOW:
				_SHOW(inputHWND);
				break;
			case NAM_TRAY_ADDNEW:
				_SHOW(createNewHWND);
				SetFocus(createNewHWND_input);
				break;
			case NAM_TRAY_LISTALL:
				_SHOW(listHWND);
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

void readOldData()
{
	std::wifstream ifs("./data.txt");
	std::wstring name, action, input;
	std::wstringstream wss;
	int temp = 0;

	for (std::wstring line; std::getline(ifs, line);) {
		wss.str(line);
		for (std::wstring entry; std::getline(wss, entry, TEXT('|'));) {
			if (temp == 0) {
				name = entry;
			}
			else if (temp == 1) {
				action = entry;
			}
			else if (temp == 2) {
				input = entry;
			}

			temp = (temp + 1) % 3;
			if (temp == 0) {
				gEntries.push_back(NAM_ENTRY(name, ACTION(std::stoi(action)), input));
			}
		}
		temp = 0;
		wss.clear();
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreviInstance, LPSTR lpCmdLine, int nCmdShow)
{
	std::locale::global(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>)); // Read UTF-8 files
	readOldData();
	gDoubleClick = false;
	gIcon = ExtractIcon(hInstance, TEXT("TrayBot.exe"), 0);

	WNDCLASS wc;

	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.hIcon = gIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = TEXT("TrayBot");
	wc.lpszMenuName = 0;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClass(&wc);

	// Start window
	inputHWND = CreateWindow(TEXT("TrayBot"), TEXT("TrayBot - Input"), NULL, 10, 10, 310, 60, NULL, NULL, hInstance, NULL);
	_HIDE(inputHWND);

	createNewHWND = CreateWindow(TEXT("TrayBot"), TEXT("TrayBot - New Input Value"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 10, 10, 330, 180, NULL, NULL, hInstance, NULL);
	_HIDE(createNewHWND);

	promptHWND = CreateWindow(TEXT("TrayBot"), TEXT("TrayBot - Prompt"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 10, 10, 330, 150, NULL, NULL, hInstance, NULL);
	_HIDE(promptHWND);

	listHWND = CreateWindow(TEXT("TrayBot"), TEXT("TrayBot - List"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VSCROLL, 10, 10, 330, 250, NULL, NULL, hInstance, NULL);
	_HIDE(listHWND);
	setList();

	// Center the windows to the screen
	centerWindows();
	// Create menu for tray icon
	gTrayMenu = CreatePopupMenu();

	menuItems();
	// Create window buttons
	windowStyles();

	// Hotkey
	RegisterHotKey(inputHWND, NAM_HOTKEY_ID, MOD_ALT | MOD_CONTROL | MOD_SHIFT, 0x51);

	// Tray icon
	gNid.cbSize = sizeof(NOTIFYICONDATA);
	gNid.hWnd = inputHWND;
	gNid.uID = NAM_TRAYICON_ID;
	gNid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	gNid.uCallbackMessage = NAM_TRAYICON_CBID;
	gNid.uVersion = NOTIFYICON_VERSION_4;
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