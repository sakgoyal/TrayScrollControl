#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cguid.h>
#include <hidusage.h>
#include <iostream>
#include <print>
#include <shellapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

using std::cerr;
using std::println;

constexpr UINT WM_TRAYICON = WM_USER + 1;

static NOTIFYICONDATAW nid = {};
static bool bIsListeningInput = false;

static bool CheckIfCursorIsInTrayIconBounds(HWND hWnd) {
	NOTIFYICONIDENTIFIER niid = {
		.cbSize = sizeof(NOTIFYICONIDENTIFIER),
		.hWnd = nid.hWnd,
		.uID = nid.uID,
		.guidItem = GUID_NULL,
	};

	POINT ptCursor;
	RECT rcIcon;
	if (GetCursorPos(&ptCursor) &&
		SUCCEEDED(Shell_NotifyIconGetRect(&niid, &rcIcon)) &&
		PtInRect(&rcIcon, ptCursor)) {
		if (!bIsListeningInput) {
			RAWINPUTDEVICE rid = {
				.usUsagePage = HID_USAGE_PAGE_GENERIC,
				.usUsage = HID_USAGE_GENERIC_MOUSE,
				.dwFlags = RIDEV_INPUTSINK,
				.hwndTarget = hWnd,
			};

			if (RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
				bIsListeningInput = true;
			}
		}
		return true;
	} else {
		if (bIsListeningInput) {
			RAWINPUTDEVICE rid = {
				.usUsagePage = HID_USAGE_PAGE_GENERIC,
				.usUsage = HID_USAGE_GENERIC_MOUSE,
				.dwFlags = RIDEV_REMOVE,
				.hwndTarget = nullptr, // required when removing
			};

			RegisterRawInputDevices(&rid, 1, sizeof(rid));
			bIsListeningInput = false;
		}
		return false;
	}
}

static void ShowTrayMenu(HWND hWnd) {
	POINT pt;
	GetCursorPos(&pt);

	HMENU hMenu = CreatePopupMenu();
	AppendMenuW(hMenu, MF_STRING, 1, L"Exit");

	SetForegroundWindow(hWnd); // Required for menu to disappear correctly

	int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, nullptr);
	DestroyMenu(hMenu);

	if (cmd == 1) {
		PostMessageW(hWnd, WM_CLOSE, 0, 0);
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	CheckIfCursorIsInTrayIconBounds(hWnd);

	switch (message) {
	case WM_INPUT: {
		UINT dwSize = sizeof(RAWINPUT);
		RAWINPUT raw{};
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
			if (raw.header.dwType == RIM_TYPEMOUSE) {
				USHORT flags = raw.data.mouse.usButtonFlags;
				if (flags & RI_MOUSE_WHEEL) {
					// wheel delta is a signed SHORT in usButtonData
					short delta = (short)raw.data.mouse.usButtonData;
					println("Raw wheel: delta={}", delta);
				}
			}
		}
	} break;

	case WM_TRAYICON: {
		if (lParam == WM_RBUTTONUP) {
			ShowTrayMenu(hWnd);
		}
	} break;

	case WM_DESTROY:
		Shell_NotifyIconW(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

int main() {
	constexpr LPCWSTR szWindowClass = L"TRAYICONSCROLL";
	HINSTANCE hInstance = GetModuleHandleW(nullptr);

	WNDCLASSEXW wcex = {
		.cbSize = sizeof(WNDCLASSEXW),
		.lpfnWndProc = WndProc,
		.hInstance = hInstance,
		.lpszClassName = szWindowClass,
	};

	if (!RegisterClassExW(&wcex)) {
		println(cerr, "Failed to register window class. Error: {}", GetLastError());
		return 1;
	}

	HWND hWnd = CreateWindowExW(0, szWindowClass, nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) {
		println(cerr, "Failed to create window. Error: {}", GetLastError());
		return 2;
	}

	nid = {
		.cbSize = sizeof(NOTIFYICONDATAW),
		.hWnd = hWnd,
		.uID = 1,
		.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP,
		.uCallbackMessage = WM_TRAYICON,
		.hIcon = LoadIcon(nullptr, IDI_APPLICATION),
		.szTip = L"Scroll Tray Icon",
	};

	if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
		println(cerr, "Failed to add tray icon.");
		return 3;
	}

	MSG msg = {};
	while (GetMessageW(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return int(msg.wParam);
}
