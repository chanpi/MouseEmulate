// MouseEmulate.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "MouseEmulate.h"
#include "Receiver.h"

#define MAX_LOADSTRING		100
#define ID_SELECT_BUTTON	(WM_APP+1)
#define fAltDown			0x2000
#define cRepeatON			0x0001

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

static void ExecuteIpodStub(void);
static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam);
static void DoOperation(HWND hTargetWnd);
static HWND g_listBox = NULL;
static HWND g_hChildWnd = NULL;

static TCHAR szBuffer[1024] = {0};

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: ここにコードを挿入してください。

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
	}

	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列を初期化しています。
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MOUSEEMULATE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// アプリケーションの初期化を実行します:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MOUSEEMULATE));

	// メイン メッセージ ループ:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WSACleanup();
	OutputDebugString(_T("\nBye!!\n"));
	return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
//  コメント:
//
//    この関数および使い方は、'RegisterClassEx' 関数が追加された
//    Windows 95 より前の Win32 システムと互換させる場合にのみ必要です。
//    アプリケーションが、関連付けられた
//    正しい形式の小さいアイコンを取得できるようにするには、
//    この関数を呼び出してください。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MOUSEEMULATE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_MOUSEEMULATE);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 300, 200, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:  メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	static HWND hTargetWnd;
	RECT rect;
	int buttonWidth = 80;
	int buttonHeight = 32;

	static Receiver receiver;

	switch (message)
	{
	case WM_CREATE:
		receiver.Start(hWnd);
		ExecuteIpodStub();

		// リストボックス作成
		GetClientRect(hWnd, &rect);
		g_listBox = CreateWindow(_T("LISTBOX"), NULL,
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
			0, 0, rect.right - rect.left, rect.bottom - rect.top - buttonHeight - 15,
			hWnd, (HMENU)1, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		if (g_listBox == NULL) {
			MessageBox(hWnd, _T("[ERROR] CreateWindow : List Box"), szTitle, MB_OK);
			PostMessage(hWnd, WM_DESTROY, 0, 0);
			break;
		}
		EnumWindows(EnumWindowsProc, NULL);

		// 選択ボタン
		CreateWindowEx(0, _T("BUTTON"), _T("Select"), WS_CHILD | WS_VISIBLE,
			(rect.right - rect.left - buttonWidth)/2, 
			rect.bottom - buttonHeight - 10,
			buttonWidth, buttonHeight, hWnd, (HMENU)ID_SELECT_BUTTON,
			((LPCREATESTRUCT)lParam)->hInstance,
			NULL);

		break;

	case WM_BRIDGEMESSAGE:
		{
			int deltaX = LOWORD(wParam);
			int deltaY = HIWORD(wParam);
			int posX, posY;
			EnumChildWindows(hTargetWnd, EnumChildProc, NULL);

			RECT rect;
			GetWindowRect(hTargetWnd, &rect);
			SetWindowPos(hTargetWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(hTargetWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

			SetFocus(hTargetWnd);
			SetForegroundWindow(hTargetWnd);
			posX = rect.left + (rect.right - rect.left)/2;
			posY = rect.top + (rect.bottom - rect.top)/2;

			//PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_MENU, MAKELONG(cRepeatON, fAltDown));
			//Sleep(5);
			//PostMessage(hTargetWnd, WM_SYSKEYDOWN, 'F', MAKELONG(cRepeatON, fAltDown));
						
			//SendMessage(hTargetWnd, WM_SYSKEYDOWN, VK_MENU, 0);
			//SendMessage(hTargetWnd, WM_MBUTTONDOWN, MK_SHIFT | MK_MBUTTON, 0);
			//SendMessage(hTargetWnd, WM_MOUSEMOVE, MK_SHIFT | MK_MBUTTON, MAKELPARAM(deltaX, deltaY));
			//SendMessage(hTargetWnd, WM_MBUTTONUP, 0, 0);
			//SendMessage(hTargetWnd, WM_SYSKEYUP, VK_MENU, 0);
			
			// 親ウィンドウに送信
			//PostMessage(hTargetWnd, WM_NCHITTEST, 0, MAKELPARAM(posX, posY));
			//PostMessage(hTargetWnd, WM_MOUSEACTIVATE, (WPARAM)g_hChildWnd, MAKELPARAM(HTCLIENT, WM_LBUTTONDOWN));
			//PostMessage(hTargetWnd, WM_SETCURSOR, (WPARAM)hTargetWnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));

			PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_MENU, MAKELONG(cRepeatON, 0));
			PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_MENU, MAKELONG(cRepeatON, fAltDown));
			PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_SHIFT, MAKELONG(cRepeatON, fAltDown));
			PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_SHIFT, MAKELONG(cRepeatON, fAltDown));
			PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_SHIFT, MAKELONG(cRepeatON, fAltDown));

			// 子ウィンドウに送信
			PostMessage(g_hChildWnd, WM_LBUTTONDOWN, MK_SHIFT | MK_LBUTTON, MAKELPARAM(posX, posY));
			PostMessage(g_hChildWnd, WM_MOUSEMOVE, MK_SHIFT | MK_LBUTTON, MAKELPARAM(posX+=1, posY+=3));
			PostMessage(g_hChildWnd, WM_MOUSEMOVE, MK_SHIFT | MK_LBUTTON, MAKELPARAM(posX+=1, posY+=3));
			PostMessage(g_hChildWnd, WM_MOUSEMOVE, MK_SHIFT | MK_LBUTTON, MAKELPARAM(posX+=1, posY+=3));
			PostMessage(g_hChildWnd, WM_MOUSEMOVE, MK_SHIFT | MK_LBUTTON, MAKELPARAM(posX+=1, posY+=3));
			PostMessage(g_hChildWnd, WM_MOUSEMOVE, MK_SHIFT | MK_LBUTTON, MAKELPARAM(posX+=1, posY+=3));
			PostMessage(g_hChildWnd, WM_MOUSEMOVE, MK_SHIFT | MK_LBUTTON, MAKELPARAM(posX+=1, posY+=3));
			PostMessage(g_hChildWnd, WM_LBUTTONUP, MK_SHIFT, MAKELPARAM(posX, posY));
			
			//PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_SHIFT, MAKELONG(cRepeatON, fAltDown));
			//PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_SHIFT, MAKELONG(cRepeatON, fAltDown));
			//PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_SHIFT, MAKELONG(cRepeatON, fAltDown));
			//PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_SHIFT, MAKELONG(cRepeatON, fAltDown));
			//PostMessage(hTargetWnd, WM_SYSKEYDOWN, VK_SHIFT, MAKELONG(cRepeatON, fAltDown));

			// 親ウィンドウに送信
			PostMessage(hTargetWnd, WM_KEYUP, VK_MENU, MAKELONG(cRepeatON, 0));
			PostMessage(hTargetWnd, WM_KEYUP, VK_SHIFT, MAKELONG(cRepeatON, 0));
		}
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 選択されたメニューの解析:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		case ID_SELECT_BUTTON:
			{
				TCHAR szAppliTitle[MAX_PATH];
				int index = SendMessage(g_listBox, LB_GETCURSEL, 0, 0);
				if (index != LB_ERR) {
					SendMessage(g_listBox, LB_GETTEXT, index, (LPARAM)szAppliTitle);

					hTargetWnd = FindWindow(NULL, szAppliTitle);
					if (hTargetWnd == NULL) {
						MessageBox(hWnd, _T("NULL"), szTitle, MB_OK);
						break;
					}
				}
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		receiver.Stop();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void ExecuteIpodStub(void)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	TCHAR szFile[BUFFER_SIZE] = _T("HekeiPhoneStub.exe");

	ZeroMemory(&si, sizeof(si));
	GetStartupInfo(&si);
	ZeroMemory(&pi, sizeof(pi));
	CreateProcess(NULL, szFile, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	TCHAR szWndTitle[MAX_PATH] = {0};
	GetWindowText(hWnd, szWndTitle, MAX_PATH);
	if (!lstrcmp(szWndTitle, _T("スタート")) || !lstrcmpi(szWndTitle, _T("start")) || !lstrcmpi(szWndTitle, _T("Program Manager"))) {
		return TRUE;
	}
	if (!IsWindowVisible(hWnd)) {
		return TRUE;
	}
	if (szWndTitle[0] == _T('\0')) {
		return TRUE;
	}
	SendMessage(g_listBox, LB_ADDSTRING, 0, (LPARAM)szWndTitle);
	return TRUE;
}

BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
{
	TCHAR szWindowTitle[256];
	GetWindowText(hWnd, szWindowTitle, 256);
	//lstrcat(szBuffer, szWindowTitle);
	//lstrcat(szBuffer, _T("\n"));
	if (g_hChildWnd == NULL) {
		if (!lstrcmpi(_T("Alias.glw"), szWindowTitle)) {
			g_hChildWnd = hWnd;
		}
	}
	return TRUE;
}

void DoOperation(HWND hTargetWnd)
{

}