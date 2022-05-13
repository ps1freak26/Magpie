#include "pch.h"
#include "XamlApp.h"
#include <winrt/Windows.UI.Core.h>
#include <CoreWindow.h>


namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::UI::ViewManagement;
}


void XamlApp::_ApplyMica() const {
	auto bkgColor = _uiSettings.GetColorValue(winrt::UIColorType::Background);
	BOOL isDarkMode = bkgColor.R < 128;
	DwmSetWindowAttribute(_hwndXamlHost, DWMWA_USE_IMMERSIVE_DARK_MODE, &isDarkMode, sizeof(isDarkMode));

	if (_isWin11) {
		constexpr const DWORD DWMWA_MICA_EFFECT = 1029;
		BOOL mica = TRUE;
		DwmSetWindowAttribute(_hwndXamlHost, DWMWA_MICA_EFFECT, &mica, sizeof(mica));
	}
}

ATOM XamlApp::_RegisterWndClass(HINSTANCE hInstance, const wchar_t* className) {
	WNDCLASSEXW wcex{};

	// 背景色遵循系统主题以避免显示时闪烁
	auto bkgColor = _uiSettings.GetColorValue(winrt::UIColorType::Background);
	HBRUSH hbrBkg = CreateSolidBrush(RGB(bkgColor.R, bkgColor.G, bkgColor.B));
	
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = _WndProcStatic;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = hbrBkg;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = className;
	wcex.hIconSm = NULL;

	return RegisterClassEx(&wcex);
}


RTL_OSVERSIONINFOW GetOSVersion() {
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	if (!hNtDll) {
		return {};
	}

	auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
	if (rtlGetVersion == nullptr) {
		return {};
	}

	RTL_OSVERSIONINFOW version{};
	version.dwOSVersionInfoSize = sizeof(version);
	rtlGetVersion(&version);

	return version;
}

bool XamlApp::Initialize(HINSTANCE hInstance, const wchar_t* className, const wchar_t* title) {
	_uiSettings = winrt::UISettings();

	auto osVersion = GetOSVersion();
	_isWin11 = osVersion.dwMajorVersion == 10 && osVersion.dwMinorVersion == 0 && osVersion.dwBuildNumber >= 22000;

	_RegisterWndClass(hInstance, className);

	_hwndXamlHost = CreateWindowEx(_isWin11 ? WS_EX_NOREDIRECTIONBITMAP | WS_EX_DLGMODALFRAME : 0,
		className, _isWin11 ? L"" : title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
		nullptr, nullptr, hInstance, nullptr);

	if (!_hwndXamlHost) {
		return false;
	}

	_ApplyMica();
	_colorChangedRevoker = _uiSettings.ColorValuesChanged(
		winrt::auto_revoke,
		[this](winrt::UISettings const& sender, winrt::IInspectable const& args) {
			_ApplyMica();
		}
	);

	return true;
}

void XamlApp::Show(winrt::Windows::UI::Xaml::UIElement xamlElement) {
	_xamlElement = xamlElement;

	// 在 Win10 上可能导致任务栏出现空的 DesktopWindowXamlSource 窗口
	// 见 https://github.com/microsoft/microsoft-ui-xaml/issues/6490
	ShowWindow(_hwndXamlHost, SW_SHOW);

	_xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();
	_xamlSourceNative2 = _xamlSource.as<IDesktopWindowXamlSourceNative2>();

	auto interop = _xamlSource.as<IDesktopWindowXamlSourceNative>();
	interop->AttachToWindow(_hwndXamlHost);

	interop->get_WindowHandle(&_hwndXamlIsland);
	_xamlSource.Content(xamlElement);

	_OnResize();

	// 防止关闭时出现 DesktopWindowXamlSource 窗口
	auto coreWindow = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
	if (coreWindow) {
		HWND hwndDWXS;
		coreWindow.as<ICoreWindowInterop>()->get_WindowHandle(&hwndDWXS);
		ShowWindow(hwndDWXS, SW_HIDE);
	}
}

int XamlApp::Run() {
	MSG msg;

	// 主消息循环
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (_xamlSource) {
			BOOL processed = FALSE;
			HRESULT hr = _xamlSourceNative2->PreTranslateMessage(&msg, &processed);
			if (SUCCEEDED(hr) && processed) {
				continue;
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

void XamlApp::_OnResize() {
	RECT clientRect;
	GetClientRect(_hwndXamlHost, &clientRect);
	SetWindowPos(_hwndXamlIsland, NULL, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW);
}

LRESULT XamlApp::_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_SIZE:
		_OnResize();
		return 0;
	case WM_SETFOCUS:
		if (_hwndXamlIsland) {
			SetFocus(_hwndXamlIsland);
		}
		return 0;
	case WM_DESTROY:
		_xamlSourceNative2 = nullptr;
		_xamlSource.Close();
		_xamlSource = nullptr;
		_xamlElement = nullptr;
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
