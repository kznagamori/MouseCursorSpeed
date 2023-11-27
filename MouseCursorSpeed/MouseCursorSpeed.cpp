// MouseCursorSpeed.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <shlwapi.h>
#include <Windows.h>
#include <direct.h>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <functional>
#include <errno.h>

#pragma comment(lib, "Shlwapi.lib")

#ifdef _UNICODE
#define SIZEOF(x)		(sizeof(x)/sizeof(x[0]))
#else
#pragma comment("UNIOCDEのみ対応")
#endif

#define STR_LEN		(256)

#define TIMER_ID	(10011)


static HINSTANCE gs_hInstance;
static INT gs_iDispWidth;
static INT gs_iDispHeight;
static UINT gs_uiSetMouseSpeed = 0;
static UINT gs_uiShowTime = 5000;

static LRESULT CALLBACK  WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static ATOM InitApp(HINSTANCE hInstance);
static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);

static DWORD GetModuleDirNameW(HMODULE  hModule, LPTSTR    pFilename, DWORD    nSize);
static WCHAR szClassName[] = L"MouseCursorSpeed";

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UINT uiMouseSpeed = 0, uiDefaultSpeed = 10;
	WCHAR AppDir[MAX_PATH + 1] = { 0 };
	WCHAR IniPath[MAX_PATH + 1] = { 0 };
	WCHAR DefaultSpped[STR_LEN] = { 0 };
	WCHAR Sppeds[STR_LEN] = { 0 };
	WCHAR ShowTime[STR_LEN] = { 0 };

	std::vector<UINT> speed_list;
	int nArgc;
	std::vector<std::wstring> args;
	gs_hInstance = hInstance;
	WCHAR* lpCommandLine = GetCommandLineW();
	WCHAR** lppArgv = ::CommandLineToArgvW(lpCommandLine, &nArgc);

	for (int i = 0; i < nArgc; ++i)
	{
		args.push_back(std::wstring(lppArgv[i]));
	}
	LocalFree(lppArgv);


	if (GetModuleDirNameW(NULL, AppDir, SIZEOF(AppDir)) == 0)
	{
		perror("GetModuleDirNameW error");
		return 1;
	}
	ZeroMemory(&IniPath[0], sizeof(IniPath));
	if (PathCombineW(IniPath, AppDir, L"MouseCursorSpeed.ini") == NULL)
	{
		perror("PathCombineW error");
		return 2;
	}

	if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, (PVOID)&uiMouseSpeed, 0) == FALSE)
	{
		perror("SystemParametersInfo error");
		return 3;
	}
	if (GetPrivateProfileString(L"PARAMS", L"DEFAULT_SPPED", L"0", DefaultSpped, SIZEOF(DefaultSpped), IniPath))
	{

		if (StrCmpW(DefaultSpped, L"0") == 0)
		{
			errno_t err = 0;
			WCHAR	pszMouseSpeed[STR_LEN];
			if ((err = _itow_s(uiMouseSpeed, pszMouseSpeed, SIZEOF(pszMouseSpeed), 10)) != 0)
			{
				perror("_itow_s error");
				return 4;
			}
			if (WritePrivateProfileString(L"PARAMS", L"DEFAULT_SPPED", pszMouseSpeed, IniPath) == FALSE)
			{
				perror("WritePrivateProfileString error");
				return 5;
			}
		}
		else
		{
			UINT val = (UINT)_wtoi(DefaultSpped);
			if (errno != 0)
			{
				perror("_wtoi");
				return 6;
			}
			if (val != 0)
			{
				uiDefaultSpeed = val;
				speed_list.push_back(val);
			}
		}
	}

	if (GetPrivateProfileString(L"PARAMS", L"SHOW_TIME", L"5000", ShowTime, SIZEOF(ShowTime), IniPath))
	{
		UINT val = (UINT)_wtoi(ShowTime);
		if (errno != 0)
		{
			perror("_wtoi");
			return 7;
		}
		if (val > 0)
		{
			gs_uiShowTime = val;
		}
	}

	if (GetPrivateProfileString(L"PARAMS", L"SPEEDS", L"0", Sppeds, SIZEOF(Sppeds), IniPath))
	{
		std::wstring speeds = std::wstring(Sppeds);
		std::wstring str = L"the;quick;brown;fox", temp;
		std::vector<std::wstring> csv_list;
		std::wstringstream wss(speeds);
		while (std::getline(wss, temp, L','))
		{
			csv_list.push_back(temp);
		}
		for (UINT i = 0; i < csv_list.size(); ++i)
		{
			errno_t err = 0;
			bool has_value = false;
			UINT val = (UINT)_wtoi(csv_list[i].c_str());
			if (errno != 0)
			{
				perror("_wtoi");
				return 8;
			}
			for (UINT j = 0; j < speed_list.size(); ++j)
			{
				if (speed_list[j] == val)
				{
					has_value = true;
					break;
				}
			}
			if (val > 0 && has_value == false)
			{
				speed_list.push_back(val);
			}
		}
		std::sort(speed_list.begin(), speed_list.end());
	}
	UINT uiNextuiMouseSpeed = speed_list[0];
	for (UINT i = 0; i < speed_list.size(); ++i)
	{
		if (speed_list[i] > uiMouseSpeed)
		{
			uiNextuiMouseSpeed = speed_list[i];
			break;
		}
	}

	if (args.size() <= 1)
	{
		if (SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID*)uiNextuiMouseSpeed, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) == FALSE)
		{
			perror("SystemParametersInfo error");
			return 9;
		}
		gs_uiSetMouseSpeed = uiNextuiMouseSpeed;
	}
	else if (args[1] == L"+")
	{
		if (uiMouseSpeed < 20)
		{
			uiMouseSpeed += 1;
		}
		if (SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID*)uiMouseSpeed, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) == FALSE)
		{
			perror("SystemParametersInfo error");
			return 9;
		}
		gs_uiSetMouseSpeed = uiMouseSpeed;
	}
	else if (args[1] == L"-")
	{
		if (uiMouseSpeed > 0)
		{
			uiMouseSpeed -= 1;
		}
		if (SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID*)uiMouseSpeed, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) == FALSE)
		{
			perror("SystemParametersInfo error");
			return 9;
		}
		gs_uiSetMouseSpeed = uiMouseSpeed;
	}
	else if (args[1] == L"d")
	{
		if (SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID*)uiDefaultSpeed, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) == FALSE)
		{
			perror("SystemParametersInfo error");
			return 9;
		}
		gs_uiSetMouseSpeed = uiDefaultSpeed;
	}
	else
	{
		perror("Unknown error");
		return 10;
	}
	MSG msg;
	BOOL bRet;
	if (!InitApp(hInstance))
	{
		perror("RegisterClassEx error");
		return 11;
	}
	if (!InitInstance(hInstance, nCmdShow))
	{
		perror("CreateWindowEx error");
		return 11;
	}
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}


static DWORD GetModuleDirNameW(HMODULE hModule, LPTSTR lpDirName, DWORD nSize)
{
	errno_t err = 0;
	TCHAR path[MAX_PATH + 1];
	GetModuleFileName(hModule, path, SIZEOF(path));

	WCHAR drive[_MAX_DRIVE];
	WCHAR dir[_MAX_DIR];
	WCHAR fname[_MAX_FNAME];
	WCHAR ext[_MAX_EXT];
	if ((err = _wsplitpath_s(path, drive, SIZEOF(drive), dir, SIZEOF(dir), fname, SIZEOF(fname), ext, SIZEOF(ext))) != 0)
	{
		perror("_wsplitpath_s error");
		return 0;
	}
	if ((err = _wmakepath_s(lpDirName, nSize, drive, dir, L"", L"")) != 0)
	{
		perror("_wmakepath_s error");
		return 0;
	}
	return lstrlenW(lpDirName);
}

static ATOM InitApp(HINSTANCE hInstance)
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;    //プロシージャ名
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;//インスタンス
	wc.hIcon = (HICON)LoadImage(NULL,
		MAKEINTRESOURCE(IDI_APPLICATION),
		IMAGE_ICON,
		0,
		0,
		LR_DEFAULTSIZE | LR_SHARED);
	wc.hCursor = (HCURSOR)LoadImage(NULL,
		MAKEINTRESOURCE(IDC_ARROW),
		IMAGE_CURSOR,
		0,
		0,
		LR_DEFAULTSIZE | LR_SHARED);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;    //メニュー名
	wc.lpszClassName = (LPCWSTR)szClassName;
	wc.hIconSm = (HICON)LoadImage(NULL,
		MAKEINTRESOURCE(IDI_APPLICATION),
		IMAGE_ICON,
		0,
		0,
		LR_DEFAULTSIZE | LR_SHARED);

	return (RegisterClassEx(&wc));
}
static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	gs_iDispWidth = GetSystemMetrics(SM_CXSCREEN);
	gs_iDispHeight = GetSystemMetrics(SM_CYSCREEN);
	hWnd = CreateWindowEx(WS_EX_LAYERED,
		szClassName,
		szClassName, //タイトルバーにこの名前が表示されます
		WS_POPUP, //ウィンドウの種類
		0,    //Ｘ座標
		0,    //Ｙ座標
		gs_iDispWidth,    //幅
		gs_iDispHeight,    //高さ
		NULL, //親ウィンドウのハンドル、親を作るときはNULL
		NULL, //メニューハンドル、クラスメニューを使うときはNULL
		hInstance, //インスタンスハンドル
		NULL);
	if (!hWnd)
	{
		return FALSE;
	}
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	return TRUE;
}

static LRESULT CALLBACK  WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int id;
	HDC hdc, hdc_mem;
	HBRUSH hBrush;
	PAINTSTRUCT ps;
	WCHAR szBuf[32] = { 0 };
	HFONT hFont;
	RECT rect;
	switch (uMsg) {
	case WM_CREATE:
		SetLayeredWindowAttributes(hWnd, RGB(255, 0, 0), 0, LWA_COLORKEY);
		//SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		SetTimer(hWnd, TIMER_ID, gs_uiShowTime, NULL);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		hBrush = CreateSolidBrush(RGB(255, 0, 0));
		SelectObject(hdc, hBrush);
		ExtFloodFill(hdc, 1, 1, RGB(255, 255, 255), FLOODFILLSURFACE);
		SetBkMode(hdc, TRANSPARENT);

		SetTextColor(hdc, RGB(0, 255, 0));  //文字の色を設定
		hFont = CreateFont(gs_iDispHeight/3, 0, //高さ, 幅
			0, 0, FW_BOLD,  //傾き(反時計回り), 文字の向き, 太字(0にするとデフォルト)
			FALSE, FALSE, FALSE,   //Italic,  下線,  打消し線
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, //キャラクタセット, 物理フォントの検索方法
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  //はみ出した場合の処理,  属性
			VARIABLE_PITCH | FF_ROMAN, NULL);    //ピッチ、ファミリ,  タイプフェイスのポインタ
		wsprintfW(szBuf, L"%d", gs_uiSetMouseSpeed);
		SelectObject(hdc, hFont);
		GetClientRect(hWnd, &rect);
		DrawText(hdc, szBuf, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		DeleteObject(hBrush);
		DeleteObject(hFont);
		EndPaint(hWnd, &ps);
		break;
	case WM_TIMER:
		if (wParam == TIMER_ID)
		{
			PostQuitMessage(0);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}
