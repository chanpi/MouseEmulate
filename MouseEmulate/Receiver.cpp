#include "StdAfx.h"
#include "Receiver.h"
#include <process.h>

#define APP_NAME	_T("MOUSE Emulate")
static SOCKET g_sock = 0;
static WSAEVENT g_stopEvent = NULL;
static HANDLE g_hThread = NULL;
static HWND g_hWnd = NULL;

static unsigned __stdcall ThreadProc(void* pParam);
static unsigned __stdcall ChildThreadProc(void* pParam);
static LPCSTR AnalyzeMessage(LPCSTR lpszMessage, LPSTR lpszCommand, SIZE_T size, int* pDeltaX, int* pDeltaY);

typedef struct {
	SOCKET client;
} ChildContext;

Receiver::Receiver(void)
{
}


Receiver::~Receiver(void)
{
}

void Receiver::Start(HWND hMainWnd)
{
	TCHAR szError[BUFFER_SIZE];

	SOCKADDR_IN address;
	UINT uThreadID;
	int error;

	g_hWnd = hMainWnd;

	g_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_sock == INVALID_SOCKET) {
		_stprintf(szError, _T("[ERROR] socket : %d"), WSAGetLastError());
		MessageBox(g_hWnd, szError, APP_NAME, MB_OK);
		return;
	}
	address.sin_family = AF_INET;
	address.sin_port = htons(12345);
	address.sin_addr.S_un.S_addr = INADDR_ANY;

	error = bind(g_sock, (SOCKADDR*)&address, sizeof(address));
	if (error) {
		MessageBox(g_hWnd, _T("[ERROR] bind"), APP_NAME, MB_OK);
		closesocket(g_sock);
		g_sock = 0;
		return;
	}
	error = listen(g_sock, 5);
	if (error) {
		MessageBox(g_hWnd, _T("[ERROR] listen"), APP_NAME, MB_OK);
		closesocket(g_sock);
		g_sock = 0;
		return;
	}

	g_stopEvent = WSACreateEvent();
	g_hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, NULL, 0, &uThreadID);
}

void Receiver::Stop(void)
{
	if (g_stopEvent != NULL) {
		WSASetEvent(g_stopEvent);
	}
	if (g_hThread != NULL) {
		WaitForSingleObject(g_hThread, INFINITE);
		CloseHandle(g_hThread);
	}
	if (g_sock != 0) {
		closesocket(g_sock);
	}
}

unsigned __stdcall ThreadProc(void* pParam)
{
	OutputDebugString(_T("Start!!!!!!!!!!!!!!!!!!!!!!!!\n"));
	TCHAR szError[BUFFER_SIZE];
	char szCommand[BUFFER_SIZE];

	SOCKET newClient;
	SOCKADDR_IN address;
	int nLen;

	HANDLE hChildThread;
	UINT uThreadID;

	DWORD dwResult = 0;
	WSAEVENT hEvent = NULL;
	WSAEVENT hEventArray[2] = {0};
	WSANETWORKEVENTS events = {0};

	hEvent = WSACreateEvent();
	if (hEvent == WSA_INVALID_EVENT) {
		wsprintf(szError, TEXT("[ERROR] WSACreateEvent(). Error No. %d"), GetLastError());
		MessageBox(NULL, szError, APP_NAME, MB_OK);
		return FALSE;
	}

	WSAEventSelect(g_sock, hEvent, FD_ACCEPT | FD_CLOSE);

	hEventArray[0] = hEvent;
	hEventArray[1] = g_stopEvent;

	for (;;) {
		dwResult = WSAWaitForMultipleEvents(2, hEventArray, FALSE, WSA_INFINITE, FALSE);
		if (dwResult == WSA_WAIT_FAILED) {
			wsprintf(szError, TEXT("[ERROR] WSA_WAIT_FAILED. Error No. %d"), GetLastError());
			MessageBox(NULL, szError, APP_NAME, MB_OK);
			break;
		}

		if (dwResult - WSA_WAIT_EVENT_0 == 0) {
			WSAEnumNetworkEvents(g_sock, hEvent, &events);
			if (events.lNetworkEvents & FD_CLOSE) {
				break;
			} else if (events.lNetworkEvents & FD_ACCEPT) {
				nLen = sizeof(address);
				newClient = accept(g_sock, (SOCKADDR*)&address, &nLen);
				if (newClient == INVALID_SOCKET) {
					break;
				}
				
				ChildContext* pContext = (ChildContext*)calloc(1, sizeof(ChildContext));
				pContext->client = newClient;
				hChildThread = (HANDLE)_beginthreadex(NULL, 0, ChildThreadProc, pContext, 0, &uThreadID);
				if (hChildThread == NULL) {
					MessageBox(NULL, TEXT("[ERROR] Create child thread"), APP_NAME, MB_OK);
					free(pContext);
					return FALSE;
				}
				CloseHandle(hChildThread);
			}
			WSAResetEvent(hEvent);
		} else if (dwResult - WSA_WAIT_EVENT_0 == 1) {
			break;
		}
	}
	WSACloseEvent(hEvent);
	shutdown(g_sock, SD_BOTH);
	closesocket(g_sock);

	return TRUE;
}

unsigned __stdcall ChildThreadProc(void* pParam)
{
	OutputDebugString(_T("Child!!!!!!!!!!!!!!!!!!!!!!!!\n"));

	ChildContext* pContext = (ChildContext*)pParam;

	int nResult = 0, nReadBytes = 0, nCommandLength = 0;
	char szRecvBuffer[BUFFER_SIZE] = {0};
	char cTermination = '?';
	TCHAR szError[BUFFER_SIZE] = {0};
	BOOL bBreak = FALSE;	// send()エラーからbreakするのに使用
	char recvBuffer[BUFFER_SIZE];
	SIZE_T totalRecvBytes = 0;
	int nBytes = 0;

	char szCommand[BUFFER_SIZE] = {0};
	int deltaX;
	int deltaY;

	DWORD dwResult = 0;
	WSAEVENT hEvent = NULL;
	WSAEVENT hEventArray[2] = {0};
	WSANETWORKEVENTS events = {0};

	hEvent = WSACreateEvent();
	WSAEventSelect(pContext->client, hEvent, FD_READ | FD_CLOSE);

	hEventArray[0] = hEvent;
	hEventArray[1] = g_stopEvent;

	for (;;) {
		dwResult = WSAWaitForMultipleEvents(2, hEventArray, FALSE, WSA_INFINITE, FALSE);
		if (dwResult == WSA_WAIT_FAILED) {
			break;
		}

		if (dwResult - WSA_WAIT_EVENT_0 == 0) {
			if (WSAEnumNetworkEvents(pContext->client, hEvent, &events) != 0) {
				_stprintf(szError, _T("[ERROR] WSAEnumNetworkEvents() : %d"), WSAGetLastError());
				MessageBox(NULL, szError, APP_NAME, MB_OK);
				break;
			}

			if (events.lNetworkEvents & FD_CLOSE) {
				break;
			} else if (events.lNetworkEvents & FD_READ) {
				nBytes = recv(pContext->client, recvBuffer, sizeof(recvBuffer) - totalRecvBytes, 0);

				if (nBytes == SOCKET_ERROR) {
					_stprintf(szError, _T("[ERROR] recv : %d"), WSAGetLastError());
					MessageBox(NULL, szError, APP_NAME, MB_OK);
					break;
				} else if (nBytes > 0) {
					totalRecvBytes += nBytes;

					// 終端文字が見つからなかった
					if (totalRecvBytes >= sizeof(recvBuffer) && memchr(recvBuffer+totalRecvBytes-nBytes, cTermination, nBytes) == NULL) {
						FillMemory(recvBuffer, sizeof(recvBuffer), 0xFF);
						totalRecvBytes = 0;
						continue;
					}

					// 電文解析
					deltaX = deltaY = 0;
					LPCSTR ptr = AnalyzeMessage(recvBuffer, szCommand, sizeof(szCommand), &deltaX, &deltaY);
					_stprintf(szError, _T("ptr : %p , recvBuffer + totalRecvBytes : %p\n"), ptr, recvBuffer + totalRecvBytes);
					OutputDebugString(szError);
					OutputDebugStringA(szCommand);
					_stprintf(szError, _T("\nx:%d y:%d\n"), deltaX, deltaY);
					OutputDebugString(szError);

					// コマンド送信のためSendMessage（deltaX, deltaY, szCommandが解放される前に処理を行う）
					SendMessage(g_hWnd, WM_BRIDGEMESSAGE, MAKEWPARAM(deltaX, deltaY), (LPARAM)szCommand);

					if (ptr == recvBuffer + totalRecvBytes) {
						FillMemory(recvBuffer, sizeof(recvBuffer), 0xFF);
						totalRecvBytes = 0;
					}
				}
			}

			ResetEvent(hEvent);
		} else if (dwResult - WSA_WAIT_EVENT_0 == 1) {
			// pChildContext->pContext->hStopEvent に終了イベントがセットされた
			break;
		}
	}
	WSACloseEvent(hEvent);

	// closesocket
	shutdown(pContext->client, SD_SEND);
	recv(pContext->client, recvBuffer, sizeof(recvBuffer), 0);
	shutdown(pContext->client, SD_BOTH);
	closesocket(pContext->client);

	free(pContext);
	return TRUE;
}

LPCSTR AnalyzeMessage(LPCSTR lpszMessage, LPSTR lpszCommand, SIZE_T size, int* pDeltaX, int* pDeltaY)
{
	char tempBuffer[BUFFER_SIZE];
	OutputDebugString(_T("Analyze!!!!!!!!!!!!!!!!!!!!!!!!\n"));

	sscanf(lpszMessage, "%s %d %d?", lpszCommand, pDeltaX, pDeltaY);

	OutputDebugStringA(lpszCommand);
	OutputDebugString(_T("\n"));

	return NULL;
	/*
	int index = 0;
	char tempBuffer[BUFFER_SIZE] = {0};
	const char* ptr = lpszMessage;
	ZeroMemory(lpszCommand, size);

	// コマンド読み取り
	while (isspace(*ptr)) {
		ptr++;
	}
	while (isalpha(*ptr)) {
		*lpszCommand++ = *ptr++;
	}

	// X移動量読み取り
	while (isspace(*ptr)) {
		ptr++;
	}
	while (!isspace(*ptr)) {
		tempBuffer[index++] = *ptr++;
	}
	*pDeltaX = atoi(tempBuffer);

	// Y移動量読み取り
	ZeroMemory(tempBuffer, sizeof(tempBuffer));
	index = 0;
	while (isspace(*ptr)) {
		ptr++;
	}
	while (!isspace(*ptr)) {
		tempBuffer[index++] = *ptr++;
	}
	*pDeltaY = atoi(tempBuffer);

	return ptr-1;	// 1文字読み戻し
	*/
}